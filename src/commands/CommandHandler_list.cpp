#include "CommandHandler.hpp"

#include <unistd.h>
#include "../utils/time.cpp"

/**
 * ----------------------------------------------------
 * handleRPUSH
 * ----------------------------------------------------
 * RESP command: RPUSH <list> <value> [value ...]
 *
 * Behavior:
 *   Appends one or more elements to the tail of a list.
 *   If the list does not exist, it is created automatically.
 *
 * Return:
 *   RESP Integer → the new length of the list.
 *
 * Side-effect for BLPOP:
 *   After pushing elements, the function attempts to wake
 *   any clients that are blocked (waiting via BLPOP).
*/
ExecResult CommandHandler::handleRPUSH(const std::vector<std::string_view>& args) {
    if (args.size() < 3)
        return ExecResult("-ERR wrong number of arguments for 'RPUSH'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto& list = lists[list_name];

    // Append all provided values to the list's tail
    for (size_t i = 2; i < args.size(); ++i) {
        std::string value = std::string(args[i]);
        list.PushBack(value);
    }

    int reply_len = list.Len();

    // Notify any BLPOP waiters that new data is available
    maybeWakeBlockedClients(list_name);

    return ExecResult(respInteger(reply_len), false, client_fd);
}

/**
 * ----------------------------------------------------
 * handleLPUSH
 * ----------------------------------------------------
 * RESP command: LPUSH <list> <value> [value ...]
 *
 * Behavior:
 *   Inserts one or more elements at the head of the list.
 *   Works symmetrically to RPUSH.
 *
 * Side-effect:
 *   May wake BLPOP waiters since new items became available.
*/
ExecResult CommandHandler::handleLPUSH(const std::vector<std::string_view>& args) {
    if (args.size() < 3)
        return ExecResult("-ERR wrong number of arguments for 'LPUSH'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto& list = lists[list_name];

    for (size_t i = 2; i < args.size(); ++i) {
        std::string value = std::string(args[i]);
        list.PushFront(value);
    }

    int reply_len = list.Len();

    // Attempt to service blocked BLPOP clients
    maybeWakeBlockedClients(list_name);

    return ExecResult(respInteger(reply_len), false, client_fd);
}

/**
 * ----------------------------------------------------
 * handleLRANGE
 * ----------------------------------------------------
 * RESP command: LRANGE <list> <start> <end>
 *
 * Behavior:
 *   Returns a slice of the list between the given indices.
 *   Negative indices are supported in Redis but not required
 *   for this implementation unless explicitly added.
 *
 * If the list does not exist, an empty RESP array is returned.
*/
ExecResult CommandHandler::handleLRANGE(const std::vector<std::string_view>& args) {
    if (args.size() != 4)
        return ExecResult("-ERR wrong number of arguments for 'LRANGE'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);

    // Non-existing list → return empty array
    if (it == lists.end()) {
        return ExecResult(respArray({}), false, client_fd);
    }

    int start = std::stoi(std::string(args[2]));
    int end   = std::stoi(std::string(args[3]));

    std::vector<std::string> elements = it->second.GetElementsInRange(start, end);

    return ExecResult(respArray(elements), false, client_fd);
}


/**
 * ----------------------------------------------------
 * handleLLEN
 * ----------------------------------------------------
 * RESP command: LLEN <list>
 *
 * Behavior:
 *   Returns the number of elements in a list.
 *
 * If list does not exist, length is 0 (matches Redis behavior).
*/
ExecResult CommandHandler::handleLLEN(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'LLEN'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end())
        return ExecResult(respInteger(0), false, client_fd);

    int list_len = it->second.Len();
    return ExecResult(respInteger(list_len), false, client_fd);
}

/**
 * ----------------------------------------------------
 * handleLPOP
 * ----------------------------------------------------
 * RESP command: LPOP <list> [count]
 *
 * Behavior:
 *   Pops and returns the first element (or multiple elements)
 *   from the head of the list.
 *
 * If the list is empty or does not exist:
 *   Return RESP Null Bulk → "$-1\r\n"
 *
 * Count > 1:
 *   Returns a RESP Array of popped elements.
*/
ExecResult CommandHandler::handleLPOP(const std::vector<std::string_view>& args) {
    if (args.size() < 2)
        return ExecResult("-ERR wrong number of arguments for 'LPOP'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end())
        return ExecResult(nullBulk(), false, client_fd);

    // LPOP key
    if (args.size() == 2) {
        std::string removed_element = it->second.POPFront();
        if (removed_element.empty())
            return ExecResult(nullBulk(), false, client_fd);

        return ExecResult(valueReturnResp(removed_element), false, client_fd);
    }

    // LPOP key count
    int pop_size = std::stoi(std::string(args[2]));
    std::vector<std::string> removed_elements;

    for (int i = 0; i < pop_size; ++i) {
        std::string removed_element = it->second.POPFront();
        if (removed_element.empty())
            break;
        removed_elements.push_back(removed_element);
    }

    return ExecResult(respArray(removed_elements), false, client_fd);
}


/**
 * ----------------------------------------------------
 * handleBLPOP  (Blocking POP)
 * ----------------------------------------------------
 * RESP command: BLPOP <list> <timeout>
 *
 * Behavior:
 *   - If the list has elements → pop immediately and return result.
 *   - If the list is empty     → block the client until:
 *        (a) another client pushes data (via RPUSH/LPUSH), OR
 *        (b) timeout occurs (not implemented here; timeout always 0)
 *
 * Timeout:
 *   In this stage, timeout is always "0", meaning "block indefinitely".
 *
 * Return Format:
 *   RESP Array:
 *      1) list name
 *      2) popped element
 *
 * Example:
 *   BLPOP mylist 0  → blocks until RPUSH mylist value
*/
ExecResult CommandHandler::handleBLPOP(const std::vector<std::string_view>& args) {
    // Sadece şu formu destekliyoruz: BLPOP key timeout
    if (args.size() != 3) {
        return ExecResult("-ERR wrong number of arguments for 'BLPOP'\r\n",
                          false, client_fd);
    }

    std::string list_name = std::string(args[1]);
    auto& list = lists[list_name];

    // If list is non-empty, return an immediate pop result
    if (!list.Empty()) {
        std::string value = list.POPFront();
        std::vector<std::string> resp = { list_name, value };
        return ExecResult(respArray(resp), false, client_fd);
    }

    double timeout_sec = std::stod(std::string(args[2]));
    uint64_t deadline = 0;

    if (timeout_sec > 0) 
        deadline = current_time_ms() + timeout_sec * 1000;
    // Otherwise, register client as blocked
    blockedClients[list_name].push_back({
        .fd = client_fd,
        .deadline_ms = deadline
    });

    // No response yet — EventLoop must not send anything
    return ExecResult("", false, client_fd);
}

/**
 * ----------------------------------------------------
 * maybeWakeBlockedClients
 * ----------------------------------------------------
 * Called automatically after a push operation (RPUSH/LPUSH)
 * on a list that may have blocked clients.
 *
 * Behavior:
 *   - For each waiting client (FIFO order)
 *       • Pop one element from the list
 *       • Build a BLPOP-style RESP array
 *       • Write response directly to the client's socket
 *
 * If all blocked clients are woken or list becomes empty,
 * remaining blocked waiters stay in the registry.
*/
void CommandHandler::maybeWakeBlockedClients(const std::string& list_name) {
    auto blk_it = blockedClients.find(list_name);
    if (blk_it == blockedClients.end())
        return;

    auto list_it = lists.find(list_name);
    if (list_it == lists.end())
        return;

    List& list = list_it->second;
    auto& waiters = blk_it->second;

    // Serve blocked clients in FIFO order
    while (!waiters.empty() && !list.Empty()) {
        int blocked_fd = waiters.front().fd;
        waiters.pop_front();

        std::string value = list.POPFront();

        // RESP array: [list_name, value]
        std::vector<std::string> resp = { list_name, value };
        std::string payload = respArray(resp);

        ::write(blocked_fd, payload.c_str(), payload.size());
    }

    // If no clients remain waiting, remove entry from map
    if (waiters.empty()) {
        blockedClients.erase(blk_it);
    }
}

void CommandHandler::checkTimeouts() {
    uint64_t now = current_time_ms();

    for (auto &pair : blockedClients) {
        const std::string &list_name = pair.first;
        auto &queue = pair.second;

        while(!queue.empty()) {
            auto &bc = queue.front();

            if(bc.deadline_ms == 0 || bc.deadline_ms > now)
                break;

            std::string payload = respArray({});
            ::write(bc.fd, payload.c_str(), payload.size());

            queue.pop_front();
        }
    }

    cleanup_empty_lists();
}

void CommandHandler::cleanup_empty_lists() {
    for (auto it = blockedClients.begin(); it != blockedClients.end(); ) {
        if (it->second.empty()) {
            it = blockedClients.erase(it);  
        } else {
            ++it;
        }
    }
}
