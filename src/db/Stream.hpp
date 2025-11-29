#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

struct StreamEntry {
    std::string id;
    std::vector<std::pair<std::string, std::string>> fields;
};

class Stream {
private:
    std::vector<StreamEntry> entries;
    std::unordered_map<std::string, std::size_t> idToIndex;

public:
    std::string addStream(const std::string& id,const std::vector<std::pair<std::string,std::string>>& fields);    
    const StreamEntry* getById(const std::string& id);
};