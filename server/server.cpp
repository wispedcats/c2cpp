#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main () {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        std::cerr << "error" << std::endl;
        return 1;
    }

    std::cout << "socket successfully created:  " << serverSocket  << std::endl;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(4959);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "bind to socket failed!" << std::endl;
        close(serverSocket);
        return 1; 
    }

    if (listen(serverSocket, 1) == -1) {
        std::cerr << "error while listening" << std::endl;
        close(serverSocket);
        return 1;
    }

    int communicationSocket = accept(serverSocket, nullptr, nullptr);
    if (communicationSocket == -1) {
        std::cerr << "accepting failed" << std::endl;
        close(serverSocket);
        return 1;
    }
    std::cout << "client connected successfully!" << std::endl;

    char puffer[1024] = {0};
    recv(communicationSocket, puffer, 1024, 0);
    std::cout << "message from client: " << puffer << std::endl;

    std::string message;
    std::cin >> message;

    int sendedBytes = send(communicationSocket, message.c_str(), message.length(), 0);

    if (sendedBytes == -1) {
        std::cerr << "error while sending!" << std::endl;
    } else {
        std::cout << "success " << sendedBytes << " bytes send" << std::endl; 
    };

    close(communicationSocket);
    close(serverSocket);

    return 0;
}

// notes:
/*
AF_INET = IP4 adress


*/
