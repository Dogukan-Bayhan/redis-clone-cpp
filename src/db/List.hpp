#include <deque>
#include <string>
#include <vector>

class List {
private:
    std::deque<std::string> list;
public:
    int PushBack(std::string element);
    int PushFront(std::string element);
    std::vector<std::string> GetElementsInRange(int start, int end);
};