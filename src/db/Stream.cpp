#include "./Stream.hpp"


std::string Stream::addStream(const std::string& id, const std::vector<std::pair<std::string, std::string>>& fields) {
    
    entries.push_back({
        id,
        fields
    });

    return id;
}

const StreamEntry* Stream::getById(const std::string& id) {
    auto it = idToIndex.find(id);
    if(it == idToIndex.end()) 
        return nullptr;

    return &entries[it->second];
}
