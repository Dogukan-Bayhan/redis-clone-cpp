#include "CommandHandler.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unistd.h>

/**
 * ----------------------------------------------------
 * Constructor
 * ----------------------------------------------------
 * Initializes the command dispatch table by mapping
 * uppercase RESP command names to their handler methods.
 *
 * This allows `execute()` to route incoming commands in
 * O(1) average time using an unordered_map lookup.
 *
 * All commands are normalized to uppercase before lookup,
 * ensuring case-insensitivity (e.g. "PING", "ping", "PiNg").
*/
CommandHandler::CommandHandler(RedisStore& str)
    : client_fd(-1),
      store(str)
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
        {"TYPE", &CommandHandler::handleTYPE},
        {"XADD", &CommandHandler::handleXADD},
        {"XRANGE", &CommandHandler::handleXRANGE}
    };
    
}


/**
 * ----------------------------------------------------
 * RESP Bulk String Encoder
 * ----------------------------------------------------
 * Builds a RESP-encoded bulk string:
 *   $<len>\r\n
 *   <value>\r\n
 *
 * Example:
 *   valueReturnResp("foo") → "$3\r\nfoo\r\n"
 *
 * Optimized with `reserve()` to avoid reallocations.
 */
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

/**
 * RESP Simple String: +OK\r\n style.
*/
std::string CommandHandler::simpleString(const std::string& s) {
    return "+" + s + "\r\n";
}

/**
 * RESP Integer: :123\r\n
*/
std::string CommandHandler::respInteger(long long n) {
    return ":" + std::to_string(n) + "\r\n";
}

/**
 * RESP Null Bulk String: $-1\r\n
*/
std::string CommandHandler::nullBulk() {
    return "$-1\r\n";
}

/**
 * RESP Bulk String Encoder
*/
std::string CommandHandler::respBulk(const std::string& value) {
    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
}

/**
 * RESP Array:
 *   *N\r\n
 *   $<len>\r\n<value>\r\n
 *   ...
*/
std::string CommandHandler::respArray(const std::vector<std::string>& values) {
    std::string out;
    out += "*" + std::to_string(values.size()) + "\r\n";

    for (const auto& v : values) {
        out += respBulk(v);
    }

    return out;
}

/**
 * ----------------------------------------------------
 * execute()
 * ----------------------------------------------------
 * Routes a parsed RESP command to the correct handler.
 *
 * Steps:
 *   1. Store calling client's file descriptor.
 *   2. Convert command name to uppercase.
 *   3. Lookup handler in dispatch table.
 *   4. Invoke member function pointer.
 *
 * No I/O is performed here — only command evaluation
 * and RESP response generation. EventLoop is responsible
 * for actually writing responses to clients.
 *
 * @param args Parsed RESP segments (command + arguments)
 * @param client_fd File descriptor of client issuing command
 * @return ExecResult containing reply + metadata
 */
ExecResult CommandHandler::execute(const std::vector<std::string_view>& args,
                                   int client_fd)
{
    this->client_fd = client_fd;

    // No command provided
    if (args.empty())
        return ExecResult("-ERR empty command\r\n", false, client_fd);

    // Normalize command name to uppercase
    std::string cmd(args[0]);
    for (char& c : cmd) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    // Dispatch lookup
    auto it = commandMap.find(cmd);
    if (it == commandMap.end())
        return ExecResult("-ERR unknown command\r\n", false, client_fd);

    // Invoke handler via member-function pointer
    return (this->*(it->second))(args);
}

std::string CommandHandler::respXRange(
    const std::vector<
        std::pair<
            std::string,
            std::vector<std::pair<std::string,std::string>>
        >
    >& entries
) {
    std::string out;

    // Outer array length = number of entries
    out += "*" + std::to_string(entries.size()) + "\r\n";

    for (const auto& entry : entries) {
        const std::string& id = entry.first;
        const auto& fields = entry.second;

        // Each entry itself is:
        // *2
        //   $len id
        //   *N fields
        out += "*2\r\n";

        // ID bulk
        out += "$" + std::to_string(id.size()) + "\r\n" + id + "\r\n";

        // Fields array
        out += "*" + std::to_string(fields.size() * 2) + "\r\n";

        for (const auto& kv : fields) {
            const auto& field = kv.first;
            const auto& value = kv.second;

            out += "$" + std::to_string(field.size()) + "\r\n" + field + "\r\n";
            out += "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
        }
    }

    return out;
}
