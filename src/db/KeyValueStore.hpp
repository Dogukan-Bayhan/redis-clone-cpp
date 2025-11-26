#pragma once
#include <unordered_map>
#include <string>

class KeyValueStore {
private:
    std::unordered_map<std::string, std::string> store;

public:
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& out);
    bool del(const std::string& key);
};
