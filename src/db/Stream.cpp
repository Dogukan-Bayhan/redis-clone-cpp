#include "./Stream.hpp"
#include <algorithm>

StreamIdType Stream::returnStreamType(const std::string& id) {

    if (id == "*")
        return StreamIdType::AUTO_GENERATED;

    size_t pos = id.find('-');
    if (pos == std::string::npos) {
        return StreamIdType::INVALID;
    }

    std::string left  = id.substr(0, pos);
    std::string right = id.substr(pos + 1);

    if (left == "*" && right == "*")
        return StreamIdType::AUTO_GENERATED;

    if (left != "*" && right == "*") {

        for (char c : left) {
            if (!isdigit(c))
                return StreamIdType::INVALID;
        }

        return StreamIdType::AUTO_SEQUENCE;
    }

    bool left_number = !left.empty() &&
        std::all_of(left.begin(), left.end(), ::isdigit);

    bool right_number = !right.empty() &&
        std::all_of(right.begin(), right.end(), ::isdigit);

    if (left_number && right_number)
        return StreamIdType::EXPLICIT;

    return StreamIdType::INVALID;
}


bool Stream::parseIdToTwoInteger(const std::string& id, long long& ms, long long& seq) {
    size_t pos = id.find('-');

    if (pos == std::string::npos) return false;

    try {
        ms = std::stoll(id.substr(0, pos));
        seq = std::stoll(id.substr(pos + 1));
    } catch(...) {
        return false;
    }

    return true;
}

bool Stream::validateId(const std::string& id, std::string& err) {
    long long ms, seq;

    if (!parseIdToTwoInteger(id, ms, seq)) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    if(ms == 0 && seq == 0) {
        err = "-ERR The ID specified in XADD must be greater than 0-0\r\n";
        return false;
    }
    
    if (entries.empty())
        return true;
    
    StreamEntry& lastStream = entries.back();

    long long last_ms, last_seq;

    parseIdToTwoInteger(lastStream.id, last_ms, last_seq);
    
    if(ms < last_ms) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;    
    } 

    if (ms == last_ms && seq <= last_seq) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    return true;
    
}


std::string Stream::addStream(const std::string& id, const std::vector<std::pair<std::string, std::string>>& fields) {
    entries.push_back({
        id,
        fields
    });
    idToIndex[id] = entries.size() - 1;
    return id;
}

const StreamEntry* Stream::getById(const std::string& id) {
    auto it = idToIndex.find(id);
    if(it == idToIndex.end()) 
        return nullptr;

    return &entries[it->second];
}

bool Stream::addSequenceToId(std::string& id, std::string& err) {
    // ID format: "<ms>-*"
    size_t pos = id.find('-');
    if (pos == std::string::npos) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    // Extract milliseconds part
    std::string ms_str = id.substr(0, pos);
    long long new_ms;

    try {
        new_ms = std::stoll(ms_str);
    } catch (...) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    // --- STREAM EMPTY CASE ---
    if (entries.empty()) {
        id = ms_str + "-1";  // first seq always 0
        return true;
    }

    // --- GET LAST ENTRY ---
    StreamEntry& last = entries.back();

    long long last_ms, last_seq;
    if (!parseIdToTwoInteger(last.id, last_ms, last_seq)) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    // --- COMPARISON RULES (Redis behavior) ---

    // If new_ms < last_ms → always invalid
    if (new_ms < last_ms) {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    long long new_seq;

    if (new_ms > last_ms) {
        // fresh millisecond → seq resets to 0
        new_seq = 0;
    } else {
        // same ms → seq increments
        new_seq = last_seq + 1;
    }

    // Build final ID
    id = ms_str + "-" + std::to_string(new_seq);
    return true;
}




