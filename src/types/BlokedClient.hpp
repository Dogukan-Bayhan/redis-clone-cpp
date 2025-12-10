#pragma once
#include <cstdint>

struct BlockedClient {
    int fd;
    uint64_t deadline_ms;
};

struct BlockedXReadClient {
    int fd;
    uint64_t deadline_ms;
    std::string stream_name;
    std::string last_id;
};