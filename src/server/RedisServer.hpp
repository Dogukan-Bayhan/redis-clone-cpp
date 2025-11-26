#pragma once

class RedisServer {
    int port;
public:
    RedisServer(int port);
    void start();
};
