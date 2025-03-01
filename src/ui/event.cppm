module;
#include <string>

export module ui.event;

export struct Event {
    std::string type;
    int x = 0;
    int y = 0;
    
    Event(std::string type) : type(std::move(type)) {}
    Event(std::string type, int x, int y) : type(std::move(type)), x(x), y(y) {}
};