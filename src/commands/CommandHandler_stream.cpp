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
