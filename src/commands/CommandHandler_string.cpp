#include "CommandHandler.hpp"



/**
 * ----------------------------------------------------
 * handlePING
 * ----------------------------------------------------
 * RESP command: PING
 * 
 * Behavior:
 *   Responds with a RESP Simple String: "+PONG\r\n".
 *
 * This is typically used by clients to verify that the
 * server is alive and able to process commands.
 *
 * No arguments are expected. Any extra arguments are ignored,
 * matching Redis' permissive behavior.
 */
ExecResult CommandHandler::handlePING(const std::vector<std::string_view>&) {
    return ExecResult(simpleString("PONG"), false, client_fd);
}

/**
 * ----------------------------------------------------
 * handleECHO
 * ----------------------------------------------------
 * RESP command: ECHO <message>
 *
 * Behavior:
 *   Returns the message back to the client as a RESP Bulk String.
 * 
 * Example:
 *   > ECHO hello
 *   < $5\r\nhello\r\n
 *
 * Error Handling:
 *   - Requires exactly 1 argument.
 */
ExecResult CommandHandler::handleECHO(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'ECHO'\r\n",
                          false, client_fd);

    return ExecResult(valueReturnResp(std::string(args[1])), false, client_fd);
}

/**
 * ----------------------------------------------------
 * handleSET
 * ----------------------------------------------------
 * RESP command: 
 *    SET <key> <value>
 *    SET <key> <value> PX <ttl_ms>
 *
 * Behavior:
 *   Stores a string value in the KeyValueStore.
 *   Returns "+OK\r\n" on success.
 *
 * TTL Variant:
 *   SET key value PX 1000
 *   → Sets key with a 1000-millisecond expiration time.
 *
 * Supported Syntax:
 *   1) SET key value
 *   2) SET key value PX ttl
 *
 * Error Handling:
 *   - Anything else results in a syntax error, matching Redis behavior.
 */
ExecResult CommandHandler::handleSET(const std::vector<std::string_view>& args) {
    if (args.size() == 3) {
        std::string key = std::string(args[1]);
        std::string val = std::string(args[2]);

        store.setString(key, val);
        return ExecResult(simpleString("OK"), false, client_fd);
    }

    if (args.size() == 5 && args[3] == "PX") {
        std::string key = std::string(args[1]);
        std::string val = std::string(args[2]);
        uint64_t ttl = std::stoull(std::string(args[4]));

        store.setString(key, val, ttl);
        return ExecResult(simpleString("OK"), false, client_fd);
    }

    return ExecResult("-ERR syntax error\r\n", false, client_fd);
}

/**
 * ----------------------------------------------------
 * handleGET
 * ----------------------------------------------------
 * RESP command: GET <key>
 *
 * Behavior:
 *   Retrieves a value from the KeyValueStore.
 *
 * Return Values:
 *   - If key exists:
 *       Bulk string containing stored value.
 *
 *   - If key does NOT exist:
 *       RESP Null Bulk → "$-1\r\n"
 *
 * Example:
 *   > SET x hello
 *   > GET x
 *   < $5\r\nhello\r\n
 *
 * Error Handling:
 *   - Requires exactly 1 argument.
 */
ExecResult CommandHandler::handleGET(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'GET'\r\n",
                          false, client_fd);

    std::string value;
    bool found = store.getString(std::string(args[1]), value);

    // Key not found → RESP null bulk
    if (!found)
        return ExecResult(nullBulk(), false, client_fd);

    // Return value as a RESP bulk string
    return ExecResult(valueReturnResp(value), false, client_fd);
}

ExecResult CommandHandler::handleTYPE(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'TYPE'\r\n",
                          false, client_fd);

    std::string key = std::string(args[1]);

    RedisObj* obj = store.getObject(key);
    if (!obj)
        return ExecResult(simpleString("none"), false, client_fd);

    switch (obj->type) {
        case RedisType::STRING:
            return ExecResult(simpleString("string"), false, client_fd);
        case RedisType::LIST:
            return ExecResult(simpleString("list"), false, client_fd);
        case RedisType::STREAM:
            return ExecResult(simpleString("stream"), false, client_fd);
    }

    return ExecResult(simpleString("none"), false, client_fd);
}
