module;
#include <string>

export module ui.event;

export struct Event {
    std::string type;
    Event(std::string type) : type(std::move(type)) {}
};