#pragma once
#include <string>

struct ExecResult {
    std::string reply;
    bool should_write;
    int target_fd;

    ExecResult(std::string r, bool w, int fd)
        : reply(std::move(r)), should_write(w), target_fd(fd) {}
};