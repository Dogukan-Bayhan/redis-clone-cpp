#include "CommandHandler.hpp"


std::string CommandHandler::valueReturnResp(const std::string& value) {
    int size = value.size();
    std::string reply;
    reply.reserve(size + 32);
    reply += "$";

    reply += std::to_string(size);
    reply += "\r\n";
    
    reply += value;
    reply += "\r\n";

    return reply;
}

std::string CommandHandler::execute(const std::vector<std::string_view>& args) {

    if (args.size() == 1 && args[0] == "PING") {
        return std::string(CommandHandler::PONG);
    }

    if (args.size() == 2 && args[0] == "ECHO") {
        std::string_view msg = args[1];

        std::string reply;
        reply.reserve(msg.size() + 32);
        reply += "$";
        reply += std::to_string(msg.size());
        reply += "\r\n";
        reply.append(msg);
        reply += "\r\n";

        return reply;
    }

    if (args.size() == 3 && args[0] == "SET") {
        std::string key(args[1]);
        std::string value(args[2]);
        db.set(key, value);
        return std::string(CommandHandler::OK);
    }

    if (args.size() == 2 && args[0] == "GET") {
        std::string key(args[1]);
        std::string value;
        bool is_find = db.get(key, value);
        if(!is_find) return std::string(CommandHandler::NULL_BULK);
        return valueReturnResp(value);
    }

    return "-ERR unknown command\r\n";
}
