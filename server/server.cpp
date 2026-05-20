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

    // receive client message
    char puffer[1024] = {0};
    recv(communicationSocket, puffer, 1024, 0);
    std::cout << "message from client: " << puffer << std::endl;

    // send reply to client
    std::string message;
    std::cin >> message;

    int sendedBytes = send(communicationSocket, message.c_str(), message.length(), 0);

    if (sendedBytes == -1) {
        std::cerr << "error while sending!" << std::endl;
    } else {
        std::cout << "success " << sendedBytes << " bytes send" << std::endl;
    }

    // cleanup sockets
    close(communicationSocket);
    close(serverSocket);

    return 0;
}

// notes:
/*
AF_INET = IP4 adress
*/
