#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netdb.h>

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