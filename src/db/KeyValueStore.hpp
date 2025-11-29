// This Key value is used to use for string operations
// in redis store. But its now moved to the Redis Store.


// #pragma once
// #include <unordered_map>
// #include <string>
// #include <cstdint>

// class KeyValueStore {
// private:
//     std::unordered_map<std::string, std::string> store;
//     std::unordered_map<std::string, uint64_t> expires;

// public:
//     void set(const std::string& key, const std::string& value);
//     void set(const std::string& key, 
//          const std::string& value, 
//          uint64_t ttl_ms);
//     bool get(const std::string& key, std::string& out);
//     bool del(const std::string& key);
// };
