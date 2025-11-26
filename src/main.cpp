#include "server/RedisServer.hpp"


int main() {
    RedisServer server(6379);
    server.start();
    return 0;
}