1-> XREAD BLOCK <milliseconds> streams <key1> <key2> <id1> <id2>
2-> Yeni bir veri yapısı 
struct BlockedXReadClient {
    int fd;
    uint64_t deadline_ms;
    std::string stream_name;
    std::string last_id;
}

std::vector<BlockedXReadClient> blockedXReadClients;
Eğer o zaman sonrası bir veri yoksa bu bir vektöre kaydedilir ve timeout ile kontrol edilir.
3-> Time out:
void CommandHandler::checkXReadTimeouts() {
    uint64_t now = current_time_ms();

    for (auto it = blockedXReadClients.begin(); it != blockedXReadClients.end(); ) {

        // deadline == 0 → infinite block
        if (it->deadline_ms != 0 && now >= it->deadline_ms) {
            std::string resp = "*-1\r\n";  // RESP null array
            ::write(it->fd, resp.c_str(), resp.size());

            it = blockedXReadClients.erase(it);
            continue;
        }

        ++it;
    }
}
4-> Eğer timeout dolmadan XADD çağırılır ise XADD de en son kontrol ve write yapma
