#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // create client socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "error" << std::endl;
        return 1;
    }

    // configure server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4959);

    // convert address text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        std::cerr << "error: unavailable address" << std::endl;
        close(clientSocket);
        return 1;
    }

    // connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) <= -1) {
        std::cerr << "error: connection failed!" << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "connected" << std::endl;

    // send request message
    std::string message = "ping";
    int sendedBytes = send(clientSocket, message.c_str(), message.length(), 0);

    if (sendedBytes == -1) {
        std::cerr << "error while sending!" << std::endl;
    } else {
        std::cout << "success " << sendedBytes << " bytes send" << std::endl;
    }

    while (true) {
        char puffer[1024] = {0};

        // waiting until server sents something
        int receivedBytes = recv(clientSocket, puffer, 1024, 0);

        if (receivedBytes > 0) {
            std::cout << "received from server: " << puffer << std::endl;


            if (std::string(puffer) == "ping") {
                send(clientSocket, "pong", 4, 0);
            }
        } else if (receivedBytes == 0) {
            std::cout << "server closed connection" << std::endl;
            break ;
        } else {
            std::cerr << "something went wrong lol" << std::endl;
            break;
        }
    }
    
    
    close(clientSocket);
    return 0;
}

// Notes LOOK README.md for informations about my learning process and the project itself.

/*
send(
    socket = socket lol
    message = message .c_str is for converting c++ string to raw memory addresses it is a array of characters (send comes from C).
    message length = send is only a memory address so it does not know so we exactly need to tell the sytem send x bytes from this address.
    0 = this is a flag we can give some "special commands" to the system like send the data now! but 0 means send the data normal.
)
*/
