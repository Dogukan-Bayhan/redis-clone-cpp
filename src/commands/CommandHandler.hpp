#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "../db/KeyValueStore.hpp"

class CommandHandler
{
private:
    static constexpr std::string_view NULL_BULK = "$-1\r\n";
    static constexpr std::string_view PONG = "+PONG\r\n";
    static constexpr std::string_view OK = "+OK\r\n";
    KeyValueStore& db;

public:
    CommandHandler(KeyValueStore& kv) : db(kv) {}
    std::string execute(const std::vector<std::string_view> &args);
    std::string valueReturnResp(const std::string& value);
};