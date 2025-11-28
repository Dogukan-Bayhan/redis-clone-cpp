#include "CommandHandler.hpp"

#include <unistd.h>


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
