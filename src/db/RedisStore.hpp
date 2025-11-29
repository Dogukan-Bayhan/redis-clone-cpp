#pragma once

#include <unordered_map>
#include <string>
#include <cstdint>

#include "../types/RedisType.hpp"

// Central in-memory storage for all Redis objects (strings, lists, streams)
// plus TTL (PX) metadata.
class RedisStore {
public:
    // Main key → Redis object dictionary
    std::unordered_map<std::string, RedisObj> data;

    // Key → absolute expiration time in ms (since epoch)
    // Only used if PX is set. If key is not present here, it does not expire.
    std::unordered_map<std::string, uint64_t> expires;

    // --- STRING API (SET/GET/DEL compatible) ---

    // SET key value
    void setString(const std::string& key, const std::string& value);

    // SET key value PX ttl_ms
    void setString(const std::string& key,
                   const std::string& value,
                   uint64_t ttl_ms);

    // GET key
    // Returns true if a non-expired STRING key exists, false otherwise.
    bool getString(const std::string& key, std::string& out);

    // DEL key
    // Deletes any type of key (string/list/stream...). Returns true if existed.
    bool del(const std::string& key);

    // --- Helpers for non-string types (lists, streams) ---

    // Returns a reference to a List object at "key",
    // creating it and setting type=LIST if necessary.
    List& getOrCreateList(const std::string& key);

    // Returns a reference to a Stream object at "key",
    // creating it and setting type=STREAM if necessary.
    Stream& getOrCreateStream(const std::string& key);

    // Raw access to the underlying object, or nullptr if key does not exist.
    RedisObj* getObject(const std::string& key);

private:
    // Internal helper: checks TTL and deletes key if expired.
    // Returns true if key is still valid (not expired or no TTL),
    // false if it expired (and was removed).
    bool ensureNotExpired(const std::string& key);
};
