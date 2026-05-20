#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        std::cerr << "error" << std::endl;
        return 1;
    }

    std::cout << "socket successfully created: " << serverSocket << std::endl;

    // prepare server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(4959);

    // bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "bind to socket failed!" << std::endl;
        close(serverSocket);
        return 1;
    }

    // start listening
    if (listen(serverSocket, 1) == -1) {
        std::cerr << "error while listening" << std::endl;
        close(serverSocket);
        return 1;
    }

    // accept client connection
    int communicationSocket = accept(serverSocket, nullptr, nullptr);

    
    

    if (communicationSocket == -1) {
        std::cerr << "accepting failed" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "client connected successfully!" << std::endl;

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    setsockopt(communicationSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    while (true) {
        char puffer[1024] = {0};

        // receive client message 
        int receivedBytes = recv(communicationSocket, puffer, 1024, 0);

        if (receivedBytes > 0) {
            std::cout << "Received message: " << puffer << std::endl;

            std::string response = "pong";
            send(communicationSocket, response.c_str(), response.length(), 0);
        } else if (receivedBytes == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                int check = send(communicationSocket, "ping", 4, 0);

                if (check == -1) {
                    // if there is no answer we close the socket connection by stopping the loop!
                    break;
                }
            } else {
                // if something other strange is happening we also disconnect
                std::cerr << "netowrk error or smth" << std::endl;
                break;
            }
        } else if (receivedBytes == 0){
            std::cout << "client disconnected :("  << std::endl;
            break;
        }

    }

    // cleanup sockets and this shi
    close(communicationSocket);
    close(serverSocket);

    return 0;
}

// notes:
/*
AF_INET = IP4 adress
*/
