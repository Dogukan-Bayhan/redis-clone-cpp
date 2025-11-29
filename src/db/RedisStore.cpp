#include "RedisStore.hpp"
#include "../utils/time.cpp"  // assumes current_time_ms() is defined here

// Internal: check TTL and delete key if expired.
bool RedisStore::ensureNotExpired(const std::string& key) {
    auto ttl_it = expires.find(key);
    if (ttl_it == expires.end()) {
        // no TTL → always valid
        return true;
    }

    uint64_t now = current_time_ms();
    if (now >= ttl_it->second) {
        // key expired → remove both object and ttl entry
        data.erase(key);
        expires.erase(ttl_it);
        return false;
    }
    return true;
}

// ----------------------------------------------------
// STRING: SET key value
// ----------------------------------------------------
void RedisStore::setString(const std::string& key, const std::string& value) {
    RedisObj obj;
    obj.type = RedisType::STRING;
    obj.value = value;

    data[key] = std::move(obj);

    // Clear any existing TTL for this key
    expires.erase(key);
}

// ----------------------------------------------------
// STRING: SET key value PX ttl_ms
// ----------------------------------------------------
void RedisStore::setString(const std::string& key,
                           const std::string& value,
                           uint64_t ttl_ms) {
    RedisObj obj;
    obj.type = RedisType::STRING;
    obj.value = value;

    data[key] = std::move(obj);

    uint64_t now = current_time_ms();
    expires[key] = now + ttl_ms;
}

// ----------------------------------------------------
// STRING: GET key
// ----------------------------------------------------
bool RedisStore::getString(const std::string& key, std::string& out) {
    auto it = data.find(key);
    if (it == data.end())
        return false;

    // If TTL is set and expired, delete and report as not-found
    if (!ensureNotExpired(key))
        return false;

    // Key exists and is not expired, but ensure it is a STRING
    if (it->second.type != RedisType::STRING)
        return false;  // or you could raise a WRONGTYPE error elsewhere

    // Extract the string from the variant
    out = std::get<std::string>(it->second.value);
    return true;
}

// ----------------------------------------------------
// DEL key
// ----------------------------------------------------
bool RedisStore::del(const std::string& key) {
    // Remove TTL metadata first
    expires.erase(key);

    // Then remove the actual object
    auto erased = data.erase(key);
    return erased > 0;
}

// ----------------------------------------------------
// LIST helper: create or reuse a List at key
// ----------------------------------------------------
List& RedisStore::getOrCreateList(const std::string& key) {
    RedisObj& obj = data[key];

    if (obj.type != RedisType::LIST) {
        // Overwrite any previous content/type
        obj.type = RedisType::LIST;
        obj.value = List{};
    }

    // Lists should not inherit old TTL from a previous type
    expires.erase(key);

    return std::get<List>(obj.value);
}

// ----------------------------------------------------
// STREAM helper: create or reuse a Stream at key
// ----------------------------------------------------
Stream& RedisStore::getOrCreateStream(const std::string& key) {
    RedisObj& obj = data[key];

    if (obj.type != RedisType::STREAM) {
        obj.type = RedisType::STREAM;
        obj.value = Stream{};
    }

    // Clear any old TTL
    expires.erase(key);

    return std::get<Stream>(obj.value);
}

// ----------------------------------------------------
// Raw access to object
// ----------------------------------------------------
RedisObj* RedisStore::getObject(const std::string& key) {
    auto it = data.find(key);
    if (it == data.end())
        return nullptr;

    // If expired, treat as absent
    if (!ensureNotExpired(key))
        return nullptr;

    return &it->second;
}
