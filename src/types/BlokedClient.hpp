#pragma once
#include <cstdint>

struct BlockedClient {
    int fd;
    uint64_t deadline_ms;
};

