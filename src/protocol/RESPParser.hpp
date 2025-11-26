#pragma once
#include <string>
#include <string_view>
#include <vector>

class RESPParser {
public: 
    static int parseInteger(const std::string& s, int& pos);
    static void skipCRLF(const std::string& s, int& pos);
    static std::vector<std::string_view> parse(const std::string& data);
};