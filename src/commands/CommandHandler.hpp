#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "../db/KeyValueStore.hpp"
#include "../db/List.hpp"

class CommandHandler
{
public:
    CommandHandler(KeyValueStore& kv) : db(kv){}
    std::string execute(const std::vector<std::string_view> &args);
private:
    KeyValueStore& db;
    std::unordered_map<std::string, List> lists;

    using CmdFn = std::string(CommandHandler::*)(const std::vector<std::string_view>&);

    std::unordered_map<std::string, CmdFn> commandMap;

    std::string valueReturnResp(const std::string& value);
    std::string simpleString(const std::string& s);
    std::string respInteger(const long long n);
    std::string nullBulk();

    std::string handlePING(const std::vector<std::string_view>& args);
    std::string handleECHO(const std::vector<std::string_view>& args);
    std::string handleSET (const std::vector<std::string_view>& args);
    std::string handleGET (const std::vector<std::string_view>& args);
    std::string handleRPUSH (const std::vector<std::string_view>& args);
};