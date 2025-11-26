#include "RedisServer.hpp"
#include "EventLoop.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

RedisServer::RedisServer(int p) : port(p) {}

void RedisServer::start() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    EventLoop loop(server_fd);
    loop.run();
}
