#include "CommandHandler.hpp"


// ----------------------------------------------------
// Constructor: build command dispatch table
// ----------------------------------------------------
CommandHandler::CommandHandler(KeyValueStore& kv) : db(kv) {
    commandMap = {
        {"PING", &CommandHandler::handlePING},
        {"ECHO", &CommandHandler::handleECHO},
        {"SET", &CommandHandler::handleSET},
        {"GET", &CommandHandler::handleGET},
        {"RPUSH", &CommandHandler::handleRPUSH},
        {"LPUSH", &CommandHandler::handleLPUSH},
        {"LRANGE", &CommandHandler::handleLRANGE},
        {"LLEN", &CommandHandler::handleLLEN},
        {"LPOP", &CommandHandler::handleLPOP},
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

std::string CommandHandler::respBulk(const std::string& value) {
    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
}

std::string CommandHandler::respArray(const std::vector<std::string>& values) {
    std::string out;

    out += "*" + std::to_string(values.size()) + "\r\n";

    for (const auto& v : values) {
        out += respBulk(v);
    }

    return out;
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
    if (args.size() < 3)
        return "-ERR wrong number of arguments for 'RPUSH'\r\n";

    int args_size = args.size();
    int newSize;

    std::string list_name = std::string(args[1]);


    for(int i = 2; i < args.size(); i++) {
        std::string value = std::string(args[i]);

        auto& list = lists[list_name];
        newSize = list.PushBack(value);
    }
    return respInteger(newSize);
}

std::string CommandHandler::handleLPUSH(const std::vector<std::string_view>& args) {
    if (args.size() < 3)
        return "-ERR wrong number of arguments for 'LPUSH'\r\n";

    int args_size = args.size();
    int newSize;

    std::string list_name = std::string(args[1]);


    for(int i = 2; i < args.size(); i++) {
        std::string value = std::string(args[i]);

        auto& list = lists[list_name];
        newSize = list.PushFront(value);
    }
    return respInteger(newSize);
}

std::string CommandHandler::handleLRANGE(const std::vector<std::string_view>& args) {
    if (args.size() != 4)
        return "-ERR wrong number of arguments for 'LRANGE'\r\n";

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end()) {
        return respArray({});
    }

    int start = std::stoi(std::string(args[2]));
    int end = std::stoi(std::string(args[3]));

    std::vector<std::string> elements = it->second.GetElementsInRange(start,end);
    
    return respArray(elements);
}

std::string CommandHandler::handleLLEN(const std::vector<std::string_view>& args) {
    if (args.size() != 2) 
        return "-ERR wrong number of arguments for 'LLEN'\r\n";

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end())
        return respInteger(0);

    int list_len = it->second.Len();
    return respInteger(list_len);
}

std::string CommandHandler::handleLPOP(const std::vector<std::string_view>& args) {
    if (args.size() != 2) 
        return "-ERR wrong number of arguments for 'LPOP'\r\n";

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end())
        return nullBulk();

    std::string removed_element = it->second.POPFront();
    if(removed_element.size() == 0) 
        return nullBulk();

    return valueReturnResp(removed_element);
}