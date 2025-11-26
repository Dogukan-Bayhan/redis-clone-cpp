#pragma once
#include <string>
#include <string_view>
#include <vector>

class CommandHandler {
public:
    static std::string execute(const std::vector<std::string_view>& args);
};