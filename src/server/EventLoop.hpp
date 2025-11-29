#pragma once
#include <sys/select.h>
#include "../db/RedisStore.hpp"
#include "../commands/CommandHandler.hpp"

class EventLoop {
    int server_fd;
    fd_set current_fds;
    
    RedisStore str;
    CommandHandler handler; 
public:
    EventLoop(int serverFd);
    void run();
};
