#include "ping.h"

#include <string>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

int ping(std::string target, int port) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);

    if (getaddrinfo(target.c_str(), portStr.c_str(), &hints, &res) != 0) {
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