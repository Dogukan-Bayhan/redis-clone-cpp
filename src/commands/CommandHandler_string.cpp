#include "CommandHandler.hpp"


// ----------------------------------------------------
// Handlers
// ----------------------------------------------------
ExecResult CommandHandler::handlePING(const std::vector<std::string_view>&) {
    return ExecResult(simpleString("PONG"), false, client_fd);
}

ExecResult CommandHandler::handleECHO(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'ECHO'\r\n",
                          false, client_fd);

    return ExecResult(valueReturnResp(std::string(args[1])), false, client_fd);
}

ExecResult CommandHandler::handleSET(const std::vector<std::string_view>& args) {
    if (args.size() == 3) {
        db.set(std::string(args[1]), std::string(args[2]));
        return ExecResult(simpleString("OK"), false, client_fd);
    }

    // SET key value PX ttl
    if (args.size() == 5 && args[3] == "PX") {
        uint64_t ttl = std::stoull(std::string(args[4]));
        db.set(std::string(args[1]), std::string(args[2]), ttl);
        return ExecResult(simpleString("OK"), false, client_fd);
    }

    return ExecResult("-ERR syntax error\r\n", false, client_fd);
}

ExecResult CommandHandler::handleGET(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'GET'\r\n",
                          false, client_fd);

    std::string value;
    bool found = db.get(std::string(args[1]), value);

    if (!found)
        return ExecResult(nullBulk(), false, client_fd);

    return ExecResult(valueReturnResp(value), false, client_fd);
}
