#include "server/RedisServer.hpp"

#include "protocol/RESPParser.cpp"
#include "commands/CommandHandler.cpp"
#include "db/KeyValueStore.cpp"
#include "server/EventLoop.cpp"

int main() {
    RedisServer server(6379);
    server.start();
    return 0;
}