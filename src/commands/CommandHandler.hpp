#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <deque>

#include "../db/KeyValueStore.hpp"
#include "../db/List.hpp"
#include "../types/ExecResult.hpp"
#include "../types/BlokedClient.hpp"

/**
 * CommandHandler
 * ---------------
 * Central dispatcher responsible for:
 *   • Parsing RESP command arguments
 *   • Routing commands to the appropriate handler
 *   • Executing string, key-value, and list operations
 *   • Managing blocking list operations (BLPOP)
 *
 * This class does NOT perform any I/O by itself; it only generates
 * RESP-encoded responses or triggers wake-ups for blocked clients.
 */
class CommandHandler
{
public:
    explicit CommandHandler(KeyValueStore& kv);

    /**
     * Executes a parsed RESP command.
     * @param args      Parsed RESP tokens (command + arguments).
     * @param client_fd Calling client's file descriptor.
     * @return          Response payload + metadata.
    */
    ExecResult execute(const std::vector<std::string_view>& args, int client_fd);

    /**
     * Responses are returned to clients who have previously made a request but whose time has passed.
     */
    void checkTimeouts();

private:
    // File descriptor of the currently executing client.
    int client_fd{};

    // Reference to the main key-value store.
    KeyValueStore& db;

    /**
     * In-memory list storage.
     * Each list is identified by its key, similar to Redis lists.
    */
    std::unordered_map<std::string, List> lists;


    /**
     * Command function pointer type.
     * Each command handler accepts a vector of arguments and returns an ExecResult.
    */
    using CmdFn = ExecResult (CommandHandler::*)(const std::vector<std::string_view>&);

    /**
     * Command dispatch table.
     * Maps uppercase RESP command names to their handler functions.
    */
    std::unordered_map<std::string, CmdFn> commandMap;

    /**
     * Blocking client registry used for BLPOP.
     * Maps: list_name -> queue of client_fds waiting for elements.
     * The order in the deque ensures FIFO wake-up semantics
     * (first client to block is the first to be served).
    */    
    std::unordered_map<std::string, std::deque<BlockedClient>> blockedClients;

    // --------------------------------------------------------------------
    // RESP Encoding Helpers
    // --------------------------------------------------------------------

    /** Constructs a RESP Bulk String containing a value. */
    std::string valueReturnResp(const std::string& value);

    /** RESP Simple String: +OK\r\n style responses. */
    std::string simpleString(const std::string& s);

    /** RESP Integer: :123\r\n */
    std::string respInteger(long long n);

    /** RESP Null Bulk String: $-1\r\n */
    std::string nullBulk();

    /** RESP Bulk String: $len\r\nvalue\r\n */
    std::string respBulk(const std::string& value);

    /** RESP Array: *N\r\n ... */
    std::string respArray(const std::vector<std::string>& values);

    // --------------------------------------------------------------------
    // String / KV Handlers
    // --------------------------------------------------------------------
    ExecResult handlePING(const std::vector<std::string_view>& args);
    ExecResult handleECHO(const std::vector<std::string_view>& args);
    ExecResult handleSET (const std::vector<std::string_view>& args);
    ExecResult handleGET (const std::vector<std::string_view>& args);
    ExecResult handleTYPE(const std::vector<std::string_view>& args);
    
    // --------------------------------------------------------------------
    // List Handlers (Redis-style list operations)
    // --------------------------------------------------------------------
    ExecResult handleRPUSH (const std::vector<std::string_view>& args);
    ExecResult handleLPUSH (const std::vector<std::string_view>& args);
    ExecResult handleLRANGE (const std::vector<std::string_view>& args);
    ExecResult handleLLEN (const std::vector<std::string_view>& args);
    ExecResult handleLPOP (const std::vector<std::string_view>& args);
    
    /**
     * Blocking pop operation (BLPOP).
     *
     * Behavior:
     *   • If the list is non-empty → pop immediately and return value.
     *   • If the list is empty     → register the client as blocked.
     *     (No response is sent; wake-up happens on future RPUSH/LPUSH)
     */
    ExecResult handleBLPOP (const std::vector<std::string_view>& args);

    /**
     * Attempts to wake clients blocked on BLPOP.
     * Called automatically after RPUSH/LPUSH.
     * Pops one element per blocked client and sends a RESP array response.
     *
     * @param list_name Name of the list for which new elements were inserted.
     */
    void maybeWakeBlockedClients(const std::string& list_name);

    void cleanup_empty_lists();
};
