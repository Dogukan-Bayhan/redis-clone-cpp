#include "EventLoop.hpp"
#include "../protocol/RESPParser.hpp"
#include "../commands/CommandHandler.hpp"

#include <unistd.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>

EventLoop::EventLoop(int serverFd) : server_fd(serverFd), db(), handler(db){
    FD_ZERO(&current_fds);
    FD_SET(server_fd, &current_fds);
}

void EventLoop::run() {
    int max_fd = server_fd;
    char buffer[4096];

    while (true) {
        fd_set ready_fds = current_fds;

        int activity = select(max_fd + 1, &ready_fds, NULL, NULL, NULL);
        if (activity < 0) {
            std::cerr << "select error\n";
            break;
        }

        if (FD_ISSET(server_fd, &ready_fds)) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            int fd = accept(server_fd, (sockaddr*)&client_addr, &len);

            FD_SET(fd, &current_fds);
            if (fd > max_fd) max_fd = fd;
        }

        for (int fd = 0; fd <= max_fd; fd++) {

            if (fd == server_fd) continue;
            if (!FD_ISSET(fd, &ready_fds)) continue;

            int bytes = read(fd, buffer, sizeof(buffer));
            if (bytes <= 0) {
                close(fd);
                FD_CLR(fd, &current_fds);
                continue;
            }

            std::string request(buffer, bytes);

            auto args = RESPParser::parse(request);
            if (args.empty()) continue;

            std::string reply = handler.execute(args);
            write(fd, reply.c_str(), reply.size());
        }
    }
}
