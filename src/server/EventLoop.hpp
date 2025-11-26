#pragma once
#include <sys/select.h>

class EventLoop {
    int server_fd;
    fd_set current_fds;

public:
    EventLoop(int serverFd);
    void run();
};
