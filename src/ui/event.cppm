module;
#include <string>

export module ui.event;

export enum class EventType {
    MouseDown,
    MouseUp,
    ButtonClick,  // Переименовано с Click
    KeyPressEvent,  // Переименовано с KeyPress
    PaintEvent,  // Переименовано с Paint
    Unknown,
    MouseMove,
    MouseEnter,
    MouseLeave
};

export struct Event {
    EventType type;
    int x = 0;
    int y = 0;

    Event(EventType type) : type(type) {}
    Event(EventType type, int x, int y) : type(type), x(x), y(y) {}

    std::string typeToString() const {
        switch (type) {
            case EventType::MouseDown:
                return "MouseDown";
            case EventType::MouseUp:
                return "MouseUp";
            case EventType::KeyPressEvent:
                return "KeyPressEvent";
            case EventType::PaintEvent:
                return "PaintEvent";
            case EventType::MouseMove:
                return "MouseMove";
            case EventType::MouseEnter:
                return "MouseEnter";
            case EventType::MouseLeave:
                return "MouseLeave";
            case EventType::ButtonClick:
                return "ButtonClick";
            default:
                return "Unknown";
        }
    }

};