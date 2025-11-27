#include <deque>
#include <string>

class List {
private:
    std::deque<std::string> list;
public:
    int PushBack(std::string element);
    int PushFront(std::string element);
};