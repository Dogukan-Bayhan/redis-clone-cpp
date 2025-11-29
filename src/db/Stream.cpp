#include "./Stream.hpp"


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
