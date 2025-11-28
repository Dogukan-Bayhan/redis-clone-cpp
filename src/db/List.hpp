#include <deque>
#include <string>
#include <vector>

class List {
private:
    std::deque<std::string> list;
public:
    bool Empty();
    int PushBack(std::string element);
    int PushFront(std::string element);
    std::string POPBack();
    std::string POPFront();
    std::vector<std::string> GetElementsInRange(int start, int end);
    int Len();
};