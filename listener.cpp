#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>

std::atomic<bool> running(true);
std::atomic<long long> packet_count(0);
std::atomic<long long> byte_count(0);

void packet_listener(int port) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket");
        return;
    }
    
    unsigned char buffer[65536];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    while (running) {
        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, 
                             (struct sockaddr*)&addr, &addr_len);
        if (bytes > 0) {
            struct iphdr *ip = (struct iphdr*)buffer;
            int ip_header_len = ip->ihl * 4;
            struct tcphdr *tcp = (struct tcphdr*)(buffer + ip_header_len);
            
            if (ntohs(tcp->dest) == port) {
                packet_count++;
                byte_count += bytes;
            }
        }
    }
    
    close(sock);
}

void show_stats() {
    auto start = std::chrono::steady_clock::now();
    long long last_packets = 0;
    long long last_bytes = 0;
    
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        
        long long current_packets = packet_count.load();
        long long current_bytes = byte_count.load();
        
        long long pps = current_packets - last_packets;
        long long bps = (current_bytes - last_bytes) * 8;
        double mbps = bps / (1024.0 * 1024.0);
        
        std::cout << "\r[" << std::fixed << std::setprecision(0) << elapsed << "s] "
                  << "Packets: " << current_packets << " | "
                  << "PPS: " << pps << " | "
                  << std::fixed << std::setprecision(2) << mbps << " Mbps    " 
                  << std::flush;
        
        last_packets = current_packets;
        last_bytes = current_bytes;
    }
}

int main() {
    int port = 1000;
    
    std::cout << "=== MONITORING PORT " << port << " ===" << std::endl;
    std::cout << "Listening for TCP packets on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "================================" << std::endl;
    
    std::thread listener(packet_listener, port);
    std::thread stats(show_stats);
    
    std::cin.get();
    running = false;
    
    listener.join();
    stats.join();
    
    std::cout << "\n\n=== FINAL STATISTICS ===" << std::endl;
    std::cout << "Total packets: " << packet_count.load() << std::endl;
    std::cout << "Total data: " << (byte_count.load() / (1024.0 * 1024.0)) << " MB" << std::endl;
    
    return 0;
}