#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

enum class StreamIdType {
    EXPLICIT,
    AUTO_SEQUENCE,
    AUTO_GENERATED,
    INVALID
};

struct StreamEntry {
    std::string id;
    std::vector<std::pair<std::string, std::string>> fields;
};

class Stream {
private:
    std::vector<StreamEntry> entries;
    std::unordered_map<std::string, std::size_t> idToIndex;

    bool parseIdToTwoInteger(const std::string&, long long&, long long&);

public:
    StreamIdType returnStreamType(const std::string& id);
    bool validateId(const std::string& id, std::string& err);
    std::string addStream(const std::string& id,const std::vector<std::pair<std::string,std::string>>& fields);    
    const StreamEntry* getById(const std::string& id);
    bool addSequenceToId(std::string& id, std::string& err);
};