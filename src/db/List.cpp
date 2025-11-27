#include "List.hpp"

int List::PushFront(std::string element) {
    list.push_front(element);
    return list.size();
}

int List::PushBack(std::string element) {
    list.push_back(element);
    return list.size();
}

std::vector<std::string> List::GetElementsInRange(int start, int end) {
    int len = list.size();
    if (len == 0) return {};

    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    
    if (start < 0) start = 0;
    if (end < 0) end = 0;

    if (start >= len) return {};

    if (end >= len) end = len - 1;

    if (start > end) return {};

    std::vector<std::string> result;
    result.reserve(end - start + 1);

    for (int i = start; i <= end; i++) {
        result.push_back(list[i]);
    }

    return result;
}

