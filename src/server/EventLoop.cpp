#include "EventLoop.hpp"
#include "../protocol/RESPParser.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>

EventLoop::EventLoop(int serverFd)
    : server_fd(serverFd),
      str(),
      handler(str)
{
    FD_ZERO(&current_fds);
    FD_SET(server_fd, &current_fds);
}

void EventLoop::run() {
    int max_fd = server_fd;
    char buffer[4096];

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000;

    while (true) {
        fd_set ready_fds = current_fds;

        int activity = select(max_fd + 1, &ready_fds, nullptr, nullptr, &tv);
        if (activity < 0) {
            std::cerr << "select error\n";
            break;
        }

        // Yeni bağlantıları al
        if (FD_ISSET(server_fd, &ready_fds)) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            int fd = accept(server_fd, (sockaddr*)&client_addr, &len);
            if (fd < 0) {
                std::cerr << "accept error\n";
            } else {
                FD_SET(fd, &current_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }

        // Var olan client'ları işle
        for (int fd = 0; fd <= max_fd; ++fd) {
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
            if (args.empty())
                continue;

            ExecResult result = handler.execute(args, fd);

            // BLPOP blokladığında reply boş olacak → hiçbir şey yazma
            if (!result.reply.empty()) {
                ::write(fd, result.reply.c_str(), result.reply.size());
            }
        }

        handler.checkTimeouts();
    }
}
