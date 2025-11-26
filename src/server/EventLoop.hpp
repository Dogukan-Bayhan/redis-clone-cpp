#pragma once
#include <sys/select.h>
#include "../db/KeyValueStore.hpp"
#include "../commands/CommandHandler.hpp"

class EventLoop {
    int server_fd;
    fd_set current_fds;
    
    KeyValueStore db;
    CommandHandler handler; 
public:
    EventLoop(int serverFd);
    void run();
};
