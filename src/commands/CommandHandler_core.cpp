#include "CommandHandler.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unistd.h>

// ----------------------------------------------------
// Constructor: build command dispatch table
// ----------------------------------------------------
CommandHandler::CommandHandler(KeyValueStore& kv)
    : client_fd(-1),
      db(kv)
{
    commandMap = {
        {"PING",  &CommandHandler::handlePING},
        {"ECHO",  &CommandHandler::handleECHO},
        {"SET",   &CommandHandler::handleSET},
        {"GET",   &CommandHandler::handleGET},
        {"RPUSH", &CommandHandler::handleRPUSH},
        {"LPUSH", &CommandHandler::handleLPUSH},
        {"LRANGE",&CommandHandler::handleLRANGE},
        {"LLEN",  &CommandHandler::handleLLEN},
        {"LPOP",  &CommandHandler::handleLPOP},
        {"BLPOP", &CommandHandler::handleBLPOP},
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
ExecResult CommandHandler::execute(const std::vector<std::string_view>& args,
                                   int client_fd)
{
    this->client_fd = client_fd;

    if (args.empty())
        return ExecResult("-ERR empty command\r\n", false, client_fd);

    std::string cmd(args[0]);
    for (char& c : cmd) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    auto it = commandMap.find(cmd);
    if (it == commandMap.end())
        return ExecResult("-ERR unknown command\r\n", false, client_fd);

    return (this->*(it->second))(args);
}