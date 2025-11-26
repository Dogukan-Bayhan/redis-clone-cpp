#include "KeyValueStore.hpp"

void KeyValueStore::set(const std::string& key, const std::string& value) {
    store[key] = value;
}

bool KeyValueStore::get(const std::string& key, std::string& out) {
    auto it = store.find(key);
    if (it == store.end()) return false;
    out = it->second;
    return true;
}

bool KeyValueStore::del(const std::string& key) {
    return store.erase(key) > 0;
}
