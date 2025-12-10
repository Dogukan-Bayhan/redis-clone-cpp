#include "./CommandHandler.hpp"


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

    return ExecResult(valueReturnResp(id),
                          false, client_fd);
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
    // Minimum: XREAD STREAMS <stream...> <id...>
    if (args.size() < 4)
        return ExecResult("-ERR wrong number of arguments for 'XREAD'\r\n",
                          false, client_fd);

    if (args[1] != "streams")
        return ExecResult("-ERR syntax error\r\n", false, client_fd);

    // -------------------------------------------------
    // Split arguments into two groups:
    //   STREAMS  s1 s2 s3   id1 id2 id3
    // -------------------------------------------------
    int total = args.size();

    // Find the midpoint: ids begin after half
    // STREAMS s1 s2 ... id1 id2 ...
    // So number of stream names = number of ids
    int half = (total - 2) / 2;

    if ((total - 2) % 2 != 0) {
        return ExecResult("-ERR XREAD requires equal number of streams and IDs\r\n",
                          false, client_fd);
    }

    std::vector<std::string> stream_names;
    std::vector<std::string> stream_ids;

    stream_names.reserve(half);
    stream_ids.reserve(half);

    for (int i = 2; i < 2 + half; i++)
        stream_names.push_back(std::string(args[i]));

    for (int i = 2 + half; i < total; i++)
        stream_ids.push_back(std::string(args[i]));

    // Now we have:
    // streams: [s1, s2, s3]
    // ids:     [id1, id2, id3]

    // -------------------------------------------------
    // Process each stream independently
    // -------------------------------------------------
    std::vector<std::string> outerArray;  // RESP array of streams

    for (int i = 0; i < half; i++) {
        const std::string& key = stream_names[i];
        const std::string& id  = stream_ids[i];

        RedisObj* obj = store.getObject(key);
        if (!obj || obj->type != RedisType::STREAM) {
            // If the stream doesn’t exist → skip (Redis ignores missing stream)
            continue;
        }

        Stream& stream = std::get<Stream>(obj->value);

        std::string err;
        auto entries = stream.getPairsFromIdToEnd(err, id);

        if (!err.empty())
            return ExecResult(err, false, client_fd);

        if (entries.empty())
            continue;  // No results for this stream

        // Encode this stream’s result
        outerArray.push_back(respXRead(key, entries));
    }

    // No results from any stream → return nil ($-1)
    if (outerArray.empty())
        return ExecResult("$-1\r\n", false, client_fd);

    // Build final RESP array
    std::string resp;
    resp += "*" + std::to_string(outerArray.size()) + "\r\n";
    for (auto& block : outerArray)
        resp += block;

    return ExecResult(resp, false, client_fd);
}
