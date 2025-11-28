#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <deque>

#include "../db/KeyValueStore.hpp"
#include "../db/List.hpp"
#include "../types/ExecResult.hpp"

class CommandHandler
{
public:
    explicit CommandHandler(KeyValueStore& kv);

    ExecResult execute(const std::vector<std::string_view>& args, int client_fd);

private:
    int client_fd{};
    KeyValueStore& db;

    // Basit list veri yapıları
    std::unordered_map<std::string, List> lists;

    using CmdFn = ExecResult (CommandHandler::*)(const std::vector<std::string_view>&);

    std::unordered_map<std::string, CmdFn> commandMap;

    // list_name -> bu listeyi BLPOP ile bekleyen client fd'leri (sıra önemli)
    std::unordered_map<std::string, std::deque<int>> blockedClients;

    // RESP helper'lar
    std::string valueReturnResp(const std::string& value);
    std::string simpleString(const std::string& s);
    std::string respInteger(long long n);
    std::string nullBulk();
    std::string respBulk(const std::string& value);
    std::string respArray(const std::vector<std::string>& values);

    // Komut handler'ları
    ExecResult handlePING(const std::vector<std::string_view>& args);
    ExecResult handleECHO(const std::vector<std::string_view>& args);
    ExecResult handleSET (const std::vector<std::string_view>& args);
    ExecResult handleGET (const std::vector<std::string_view>& args);
    ExecResult handleRPUSH (const std::vector<std::string_view>& args);
    ExecResult handleLPUSH (const std::vector<std::string_view>& args);
    ExecResult handleLRANGE (const std::vector<std::string_view>& args);
    ExecResult handleLLEN (const std::vector<std::string_view>& args);
    ExecResult handleLPOP (const std::vector<std::string_view>& args);
    ExecResult handleBLPOP (const std::vector<std::string_view>& args);

    // RPUSH / LPUSH sonrası, bu listeyi bekleyen bloklu client'ları uyandır
    void maybeWakeBlockedClients(const std::string& list_name);
};
