#include "CommandHandler.hpp"

std::string CommandHandler::execute(const std::vector<std::string_view>& args) {

    if (args.size() == 1 && args[0] == "PING") {
        return "+PONG\r\n";
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

    return "-ERR unknown command\r\n";
}
