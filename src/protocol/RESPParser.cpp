#include "RESPParser.hpp"

int RESPParser::parseInteger(const std::string& s, int& pos) {
    int start = pos;

    while (true) {
        if (pos >= (int)s.size()) return -1;
        if (s[pos] == '\r') break;
        if (s[pos] < '0' || s[pos] > '9') return -1;
        pos++;
    }

    int num = 0;
    for (int i = start; i < pos; i++)
        num = num * 10 + (s[i] - '0');

    if (pos + 1 >= (int)s.size()) return -1;
    if (s[pos] != '\r' || s[pos + 1] != '\n') return -1;

    pos += 2;
    return num;
}

void RESPParser::skipCRLF(const std::string& s, int& pos) {
    if (pos + 1 < (int)s.size() &&
        s[pos] == '\r' && s[pos + 1] == '\n')
        pos += 2;
}

std::vector<std::string_view> RESPParser::parse(const std::string& data) {
    std::vector<std::string_view> values;

    int pos = 0;

    if (data.empty() || data[pos] != '*') return {};
    pos++;

    int count = parseInteger(data, pos);
    if (count < 0) return {};

    for (int i = 0; i < count; i++) {
        if (pos >= (int)data.size() || data[pos] != '$')
            return {};

        pos++;

        int len = parseInteger(data, pos);
        if (len < 0 || pos + len > (int)data.size()) return {};

        std::string_view word(data.data() + pos, len);
        values.push_back(word);

        pos += len;
        skipCRLF(data, pos);
    }

    return values;
}
