#include "./CommandHandler.hpp"

#include <unistd.h>

ExecResult CommandHandler::handleXADD(const std::vector<std::string_view>& args) {
    if (args.size() < 5) 
        return ExecResult("-ERR wrong number of arguments for 'XADD'\r\n",
                          false, client_fd);

    std::string stream_name = std::string(args[1]);
    Stream& stream = store.getOrCreateStream(stream_name);

    std::string id = std::string(args[2]);

    StreamIdType streamType = stream.returnStreamType(id);


    if (streamType == StreamIdType::INVALID) {
        return ExecResult(
            "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n",
            false, client_fd
        );
    }
    
    std::string err;
    if (streamType == StreamIdType::AUTO_SEQUENCE){
        if (!stream.addSequenceToId(id, err)) {
            return ExecResult(err, false, client_fd);
        }
    } else if(streamType == StreamIdType::AUTO_GENERATED) {
        if(!stream.createUniqueId(id, err)) {
            return ExecResult(err, false, client_fd);
        }
    } else {
        if (!stream.validateId(id, err)) {
            return ExecResult(err, false, client_fd);
        }
    }

    std::vector<std::pair<std::string, std::string>> fields;

    if ((args.size() - 3) < 2) {
        return ExecResult("-ERR XADD requires field-value pairs\r\n", false, client_fd);
    }

    if (((args.size() - 3) % 2) != 0) {
        return ExecResult("-ERR XADD field-value pairs are incomplete\r\n", false, client_fd);
    }

    for (int i = 3; i < args.size(); i += 2) {
        const std::string& field = std::string(args[i]);
        const std::string& value = std::string(args[i + 1]);

       
        if (field.empty() || value.empty()) {
            return ExecResult("-ERR XADD fields cannot be empty\r\n", false, client_fd);
        }

        fields.push_back({field, value});
    }

    stream.addStream(id, fields);

    wakeBlockedXReadClients(stream_name, id);

    return ExecResult(valueReturnResp(id),
                          false, client_fd);
}

void CommandHandler::wakeBlockedXReadClients(
    const std::string& stream_name,
    const std::string& new_id
) {
    std::vector<BlockedXReadClient> stillBlocked;

    for (auto& bc : blockedXReadClients) {
        if (bc.stream_name != stream_name) {
            stillBlocked.push_back(bc);
            continue;
        }

        RedisObj* obj = store.getObject(stream_name);
        if (!obj || obj->type != RedisType::STREAM) {
            continue;
        }

        Stream& stream = std::get<Stream>(obj->value);

        std::string err;
        auto entries = stream.getPairsFromIdToEnd(err, bc.last_id);
        if (!err.empty() || entries.empty()) {
            continue;
        }

        // Encode response
        std::string blockResp = wrapXReadBlocks({
            respXRead(stream_name, entries)
        });

        ::write(bc.fd, blockResp.c_str(), blockResp.size());
        // Do not re-add → remove from block list
    }

    blockedXReadClients = std::move(stillBlocked);
}


ExecResult CommandHandler::handleXRANGE(const std::vector<std::string_view>& args) {
    if (args.size() != 4) {
        return ExecResult("-ERR wrong number of arguments for 'XRANGE'\r\n",
                          false, client_fd);
    }

    std::string stream_name = std::string(args[1]);
    std::string start_id    = std::string(args[2]);
    std::string end_id      = std::string(args[3]);

    // Stream yoksa → boş array dön (Redis davranışı)
    RedisObj* obj = store.getObject(stream_name);
    if (!obj) {
        return ExecResult(respArray({}), false, client_fd);
    }

    if (obj->type != RedisType::STREAM) {
        return ExecResult("-WRONGTYPE Key is not a stream\r\n",
                          false, client_fd);
    }

    Stream& stream = std::get<Stream>(obj->value);

    std::string err;

    if (start_id == "-") {
        auto entries = stream.getPairsFromStartToId(err, end_id);
        std::string payload = respXRange(entries);
        return ExecResult(payload, false, client_fd);
    }

    if (end_id == "+") {
        auto entries = stream.getPairsFromIdToEnd(err, start_id);
        std::string payload = respXRange(entries);
        return ExecResult(payload, false, client_fd);
    }

    auto entries = stream.getPairsInRange(err, start_id, end_id);

    if (!err.empty()) {
        return ExecResult(err, false, client_fd);
    }

    // XRANGE output
    std::string payload = respXRange(entries);
    return ExecResult(payload, false, client_fd);
}

ExecResult CommandHandler::handleXREAD(const std::vector<std::string_view>& args) {
    //
    // XREAD [BLOCK ms] STREAMS key1 key2 ... id1 id2 ...
    //
    int idx = 1;
    bool is_blocking = false;
    uint64_t block_timeout = 0;

    // -------------------------------------------------
    // 1) Check BLOCK option
    // -------------------------------------------------
    if (idx < args.size() && (args[idx] == "BLOCK" || args[idx] == "block")) {
        is_blocking = true;

        if (idx + 1 >= args.size())
            return ExecResult("-ERR syntax error\r\n", false, client_fd);

        try {
            block_timeout = std::stoull(std::string(args[idx + 1]));
        } catch (...) {
            return ExecResult("-ERR invalid timeout\r\n", false, client_fd);
        }

        idx += 2; // skip BLOCK <ms>
    }

    // -------------------------------------------------
    // 2) Expect STREAMS keyword
    // -------------------------------------------------
    if (idx >= args.size() || args[idx] != "streams")
        return ExecResult("-ERR syntax error\r\n", false, client_fd);

    idx++; // move past STREAMS

    // From here: STREAMS k1 k2 ... id1 id2 ...
    int remaining = args.size() - idx;

    if (remaining < 2)
        return ExecResult("-ERR wrong number of arguments for 'XREAD'\r\n",
                          false, client_fd);

    if (remaining % 2 != 0)
        return ExecResult("-ERR XREAD requires equal number of streams and IDs\r\n",
                          false, client_fd);

    int half = remaining / 2;

    std::vector<std::string> stream_names;
    std::vector<std::string> stream_ids;

    stream_names.reserve(half);
    stream_ids.reserve(half);

    for (int i = 0; i < half; i++)
        stream_names.push_back(std::string(args[idx + i]));

    for (int i = 0; i < half; i++)
        stream_ids.push_back(std::string(args[idx + half + i]));

    // -------------------------------------------------
    // 3) Try to read immediately
    // -------------------------------------------------
    std::vector<std::string> outerArray;  

    for (int i = 0; i < half; i++) {
        const std::string &key = stream_names[i];
        const std::string &raw_id  = stream_ids[i];

        RedisObj *obj = store.getObject(key);
        if (!obj || obj->type != RedisType::STREAM)
            continue;

        Stream &stream = std::get<Stream>(obj->value);

        // GIVEN ID → increment to get exclusive start
        std::string next_id = stream.incrementId(raw_id);

        std::string err;
        auto entries = stream.getPairsFromIdToEnd(err, next_id);

        if (!err.empty())
            return ExecResult(err, false, client_fd);

        if (!entries.empty()) {
            outerArray.push_back(respXRead(key, entries));
        }
    }

    // -------------------------------------------------
    // 4) If results exist → return immediately
    // -------------------------------------------------
    if (!outerArray.empty()) {
        std::string resp = wrapXReadBlocks(outerArray);
        return ExecResult(resp, false, client_fd);
    }

    // -------------------------------------------------
    // 5) If NOT blocking → return NIL ($-1)
    // -------------------------------------------------
    if (!is_blocking) {
        return ExecResult("$-1\r\n", false, client_fd);
    }

    // -------------------------------------------------
    // 6) BLOCKING MODE → register client and RETURN EMPTY reply
    // -------------------------------------------------
    uint64_t now = current_time_ms();
    uint64_t deadline = (block_timeout == 0 ? 0 : now + block_timeout);

    for (int i = 0; i < half; i++) {
        blockedXReadClients.push_back({
            client_fd,
            deadline,
            stream_names[i],
            stream_ids[i]
        });
    }

    // EMPTY reply + should_block = true
    return ExecResult("", true, client_fd);
}



void CommandHandler::checkXReadTimeouts() {
    uint64_t now = current_time_ms();

    for (auto it = blockedXReadClients.begin(); it != blockedXReadClients.end(); ) {

        // deadline == 0 → infinite block
        if (it->deadline_ms != 0 && now >= it->deadline_ms) {
            std::string resp = "*-1\r\n";  // RESP null array
            ::write(it->fd, resp.c_str(), resp.size());

            it = blockedXReadClients.erase(it);
            continue;
        }

        ++it;
    }
}