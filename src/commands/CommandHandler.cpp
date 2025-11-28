#include "CommandHandler.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unistd.h>

// ----------------------------------------------------
// Constructor: build command dispatch table
// ----------------------------------------------------
CommandHandler::CommandHandler(KeyValueStore& kv)
    : client_fd(-1),
      db(kv)
{
    commandMap = {
        {"PING",  &CommandHandler::handlePING},
        {"ECHO",  &CommandHandler::handleECHO},
        {"SET",   &CommandHandler::handleSET},
        {"GET",   &CommandHandler::handleGET},
        {"RPUSH", &CommandHandler::handleRPUSH},
        {"LPUSH", &CommandHandler::handleLPUSH},
        {"LRANGE",&CommandHandler::handleLRANGE},
        {"LLEN",  &CommandHandler::handleLLEN},
        {"LPOP",  &CommandHandler::handleLPOP},
        {"BLPOP", &CommandHandler::handleBLPOP},
    };
}

// ----------------------------------------------------
// RESP Encoders
// ----------------------------------------------------
std::string CommandHandler::valueReturnResp(const std::string& value) {
    size_t size = value.size();
    std::string len = std::to_string(size);

    std::string reply;
    reply.reserve(1 + len.size() + 2 + size + 2);

    reply += '$';
    reply += len;
    reply += "\r\n";
    reply += value;
    reply += "\r\n";

    return reply;
}

std::string CommandHandler::simpleString(const std::string& s) {
    return "+" + s + "\r\n";
}

std::string CommandHandler::respInteger(long long n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string CommandHandler::nullBulk() {
    return "$-1\r\n";
}

std::string CommandHandler::respBulk(const std::string& value) {
    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
}

std::string CommandHandler::respArray(const std::vector<std::string>& values) {
    std::string out;
    out += "*" + std::to_string(values.size()) + "\r\n";

    for (const auto& v : values) {
        out += respBulk(v);
    }

    return out;
}

// ----------------------------------------------------
// Main Command Dispatcher 
// ----------------------------------------------------
ExecResult CommandHandler::execute(const std::vector<std::string_view>& args,
                                   int client_fd)
{
    this->client_fd = client_fd;

    if (args.empty())
        return ExecResult("-ERR empty command\r\n", false, client_fd);

    std::string cmd(args[0]);
    for (char& c : cmd) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    auto it = commandMap.find(cmd);
    if (it == commandMap.end())
        return ExecResult("-ERR unknown command\r\n", false, client_fd);

    return (this->*(it->second))(args);
}

// ----------------------------------------------------
// Handlers
// ----------------------------------------------------
ExecResult CommandHandler::handlePING(const std::vector<std::string_view>&) {
    return ExecResult(simpleString("PONG"), false, client_fd);
}

ExecResult CommandHandler::handleECHO(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'ECHO'\r\n",
                          false, client_fd);

    return ExecResult(valueReturnResp(std::string(args[1])), false, client_fd);
}

ExecResult CommandHandler::handleSET(const std::vector<std::string_view>& args) {
    if (args.size() == 3) {
        db.set(std::string(args[1]), std::string(args[2]));
        return ExecResult(simpleString("OK"), false, client_fd);
    }

    // SET key value PX ttl
    if (args.size() == 5 && args[3] == "PX") {
        uint64_t ttl = std::stoull(std::string(args[4]));
        db.set(std::string(args[1]), std::string(args[2]), ttl);
        return ExecResult(simpleString("OK"), false, client_fd);
    }

    return ExecResult("-ERR syntax error\r\n", false, client_fd);
}

ExecResult CommandHandler::handleGET(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'GET'\r\n",
                          false, client_fd);

    std::string value;
    bool found = db.get(std::string(args[1]), value);

    if (!found)
        return ExecResult(nullBulk(), false, client_fd);

    return ExecResult(valueReturnResp(value), false, client_fd);
}

ExecResult CommandHandler::handleRPUSH(const std::vector<std::string_view>& args) {
    if (args.size() < 3)
        return ExecResult("-ERR wrong number of arguments for 'RPUSH'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto& list = lists[list_name];

    // Önce tüm değerleri listeye ekle
    for (size_t i = 2; i < args.size(); ++i) {
        std::string value = std::string(args[i]);
        list.PushBack(value);
    }

    // RPUSH'in integer cevabı: push sonrası listenin uzunluğu
    int reply_len = list.Len();

    // Ardından bu listeyi bekleyen BLPOP client'larını uyandır
    maybeWakeBlockedClients(list_name);

    return ExecResult(respInteger(reply_len), false, client_fd);
}

ExecResult CommandHandler::handleLPUSH(const std::vector<std::string_view>& args) {
    if (args.size() < 3)
        return ExecResult("-ERR wrong number of arguments for 'LPUSH'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto& list = lists[list_name];

    for (size_t i = 2; i < args.size(); ++i) {
        std::string value = std::string(args[i]);
        list.PushFront(value);
    }

    int reply_len = list.Len();

    maybeWakeBlockedClients(list_name);

    return ExecResult(respInteger(reply_len), false, client_fd);
}

ExecResult CommandHandler::handleLRANGE(const std::vector<std::string_view>& args) {
    if (args.size() != 4)
        return ExecResult("-ERR wrong number of arguments for 'LRANGE'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end()) {
        return ExecResult(respArray({}), false, client_fd);
    }

    int start = std::stoi(std::string(args[2]));
    int end   = std::stoi(std::string(args[3]));

    std::vector<std::string> elements = it->second.GetElementsInRange(start, end);

    return ExecResult(respArray(elements), false, client_fd);
}

ExecResult CommandHandler::handleLLEN(const std::vector<std::string_view>& args) {
    if (args.size() != 2)
        return ExecResult("-ERR wrong number of arguments for 'LLEN'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end())
        return ExecResult(respInteger(0), false, client_fd);

    int list_len = it->second.Len();
    return ExecResult(respInteger(list_len), false, client_fd);
}

ExecResult CommandHandler::handleLPOP(const std::vector<std::string_view>& args) {
    if (args.size() < 2)
        return ExecResult("-ERR wrong number of arguments for 'LPOP'\r\n",
                          false, client_fd);

    std::string list_name = std::string(args[1]);
    auto it = lists.find(list_name);
    if (it == lists.end())
        return ExecResult(nullBulk(), false, client_fd);

    if (args.size() == 2) {
        std::string removed_element = it->second.POPFront();
        if (removed_element.empty())
            return ExecResult(nullBulk(), false, client_fd);

        return ExecResult(valueReturnResp(removed_element), false, client_fd);
    }

    int pop_size = std::stoi(std::string(args[2]));
    std::vector<std::string> removed_elements;

    for (int i = 0; i < pop_size; ++i) {
        std::string removed_element = it->second.POPFront();
        if (removed_element.empty())
            break;
        removed_elements.push_back(removed_element);
    }

    return ExecResult(respArray(removed_elements), false, client_fd);
}

// ----------------------------------------------------
// BLPOP: Blocking LPOP
// ----------------------------------------------------
ExecResult CommandHandler::handleBLPOP(const std::vector<std::string_view>& args) {
    // Sadece şu formu destekliyoruz: BLPOP key timeout
    if (args.size() != 3) {
        return ExecResult("-ERR wrong number of arguments for 'BLPOP'\r\n",
                          false, client_fd);
    }

    std::string list_name = std::string(args[1]);
    // std::string timeout_str = std::string(args[2]); // Bu aşamada her zaman "0"

    auto& list = lists[list_name];

    // Liste boş değilse, hemen POP yap ve cevap dön (bloklamaya gerek yok)
    if (!list.Empty()) {
        std::string value = list.POPFront();
        std::vector<std::string> resp = { list_name, value };
        return ExecResult(respArray(resp), false, client_fd);
    }

    // Liste boşsa, bu client'ı bloklu client listesine ekle.
    blockedClients[list_name].push_back(client_fd);

    // Şu anda cevap yazmıyoruz, RPUSH/LPUSH geldiğinde uyandırılacak.
    return ExecResult("", false, client_fd);
}

// ----------------------------------------------------
// BLPOP bekleyen client'ları uyandırma
// ----------------------------------------------------
void CommandHandler::maybeWakeBlockedClients(const std::string& list_name) {
    auto blk_it = blockedClients.find(list_name);
    if (blk_it == blockedClients.end())
        return;

    auto list_it = lists.find(list_name);
    if (list_it == lists.end())
        return;

    List& list = list_it->second;
    auto& waiters = blk_it->second;

    // Her bloklu client için, listeden bir eleman POP edip yanıtla
    while (!waiters.empty() && !list.Empty()) {
        int blocked_fd = waiters.front();
        waiters.pop_front();

        std::string value = list.POPFront();

        std::vector<std::string> resp = { list_name, value };
        std::string payload = respArray(resp);

        ::write(blocked_fd, payload.c_str(), payload.size());
    }

    if (waiters.empty()) {
        blockedClients.erase(blk_it);
    }
}
