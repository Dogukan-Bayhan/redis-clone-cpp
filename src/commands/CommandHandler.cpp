#include "CommandHandler.hpp"


// ----------------------------------------------------
// Constructor: build command dispatch table
// ----------------------------------------------------
CommandHandler::CommandHandler(KeyValueStore& kv) : db(kv) {
    commandMap = {
        {"PING", &CommandHandler::handlePING},
        {"ECHO", &CommandHandler::handleECHO},
        {"SET",  &CommandHandler::handleSET},
        {"GET",  &CommandHandler::handleGET},
        {"RPUSH",  &CommandHandler::handleRPUSH},
    };
}

// ----------------------------------------------------
// RESP Encoders
// ----------------------------------------------------
std::string CommandHandler::valueReturnResp(const std::string& value) {
    size_t size = value.size();
    std::string len = std::to_string(size);

    std::string reply;
    reply.reserve(1 + len.size() + 2 + size + 2);

    reply += '$';
    reply += len;
    reply += "\r\n";
    reply += value;
    reply += "\r\n";

    return reply;
}

std::string CommandHandler::simpleString(const std::string& s) {
    return "+" + s + "\r\n";
}

std::string CommandHandler::respInteger(long long n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string CommandHandler::nullBulk() {
    return "$-1\r\n";
}

// ----------------------------------------------------
// Main Command Dispatcher 
// ----------------------------------------------------
std::string CommandHandler::execute(const std::vector<std::string_view>& args) {
    if (args.empty()) 
        return "-ERR empty command\r\n";

    std::string cmd(args[0]);
    for (char& c : cmd) c = std::toupper(c);

    auto it = commandMap.find(cmd);
    if (it == commandMap.end())
        return "-ERR unknown command\r\n";
    
    return (this->*(it->second))(args);
}

// ----------------------------------------------------
// Handlers
// ----------------------------------------------------
std::string CommandHandler::handlePING(const std::vector<std::string_view>& args) {
    return simpleString("PONG");
}


std::string CommandHandler::handleECHO(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return "-ERR wrong number of arguments for 'ECHO'\r\n";

    return valueReturnResp(std::string(args[1]));
}


std::string CommandHandler::handleSET(const std::vector<std::string_view>& args) {
    if (args.size() == 3) {
        db.set(std::string(args[1]), std::string(args[2]));
        return simpleString("OK");
    }

    // SET key value PX ttl
    if (args.size() == 5 && args[3] == "PX") {
        uint64_t ttl = std::stoull(std::string(args[4]));
        db.set(std::string(args[1]), std::string(args[2]), ttl);
        return simpleString("OK");
    }

    return "-ERR syntax error\r\n";
}


std::string CommandHandler::handleGET(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return "-ERR wrong number of arguments for 'GET'\r\n";

    std::string value;
    bool found = db.get(std::string(args[1]), value);

    if (!found)
        return nullBulk();

    return valueReturnResp(value);
}

std::string CommandHandler::handleRPUSH(const std::vector<std::string_view>& args) {
    if (args.size() != 3)
        return "-ERR wrong number of arguments for 'RPUSH'\r\n";

    std::string list_name = std::string(args[1]);
    std::string value = std::string(args[2]);

    auto& list = lists[list_name];
    int newSize = list.PushBack(value);
    return respInteger(newSize);
}
