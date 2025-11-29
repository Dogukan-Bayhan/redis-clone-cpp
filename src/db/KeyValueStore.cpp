// #include "KeyValueStore.hpp"
// #include "../utils/time.cpp"

// void KeyValueStore::set(const std::string& key, const std::string& value) {
//     store[key] = value;
//     expires.erase(key);
// }

// void KeyValueStore::set(const std::string& key,const std::string& value, uint64_t ttl_ms) {

//     store[key] = value;

//     uint64_t now = current_time_ms();   
//     expires[key] = now + ttl_ms;
// }

// bool KeyValueStore::get(const std::string& key, std::string& out) {
//     auto it = store.find(key);
//     if (it == store.end()) return false;
    
//     auto ttl = expires.find(key);
//     if (ttl != expires.end()) {
//         uint64_t now = current_time_ms();
//         if (now >= ttl->second) {
//             store.erase(key);
//             expires.erase(key);
//             return false;
//         }
//     }
//     out = it->second;
//     return true;
// }

// bool KeyValueStore::del(const std::string& key) {
//     expires.erase(key);
//     return store.erase(key) > 0;
// }
