#include "List.hpp"

int List::PushFront(std::string element) {
    list.push_front(element);
    return list.size();
}

int List::PushBack(std::string element) {
    list.push_back(element);
    return list.size();
}

