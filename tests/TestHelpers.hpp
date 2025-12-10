#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../src/protocol/RESPParser.hpp"

/**
 * Helper container that keeps the owning std::string storage
 * next to the std::string_view arguments expected by CommandHandler.
 */
struct RespArgs {
    std::vector<std::string> storage;
    std::vector<std::string_view> views;

    RespArgs() = default;

    explicit RespArgs(std::vector<std::string> values)
        : storage(std::move(values)) {
        rebuild();
    }

    explicit RespArgs(std::initializer_list<const char*> init) {
        for (auto s : init) {
            storage.emplace_back(s);
        }
        rebuild();
    }

    void rebuild() {
        views.clear();
        views.reserve(storage.size());
        for (const auto& s : storage) {
            views.emplace_back(s);
        }
    }
};

inline RespArgs makeArgs(std::initializer_list<const char*> init) {
    return RespArgs(init);
}

inline RespArgs makeArgs(std::vector<std::string> values) {
    return RespArgs(std::move(values));
}

inline std::vector<std::string> parseBulkArray(const std::string& resp) {
    auto views = RESPParser::parse(resp);
    std::vector<std::string> out;
    out.reserve(views.size());
    for (auto v : views) {
        out.emplace_back(v);
    }
    return out;
}
