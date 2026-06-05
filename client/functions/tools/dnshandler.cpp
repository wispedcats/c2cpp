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

void dnsHandler(std::string &target) {
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(target.c_str(), NULL, &hints, &res);
  if (status != 0) {
    std::cout << "DNS error: " << gai_strerror(status) << std::endl;
    return;
  }

  char ip[INET_ADDRSTRLEN];
  struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;

  inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));

  std::cout << "IP: " << ip << std::endl;

  freeaddrinfo(res);
}