module;
#include <string>
#include <X11/Xlib.h>
#include <functional>

import ui.widget;
import ui.event;
import ui.event_listener;

export module ui.button;

export class Button : public Widget {
private:
    std::string label;

public:
    explicit Button(std::string label) : label(std::move(label)) {}
    
    void handleEvent(const Event& event) override {
        if (event.type == "click" && 
            event.x >= getX() && event.x < getX() + getWidth() &&
            event.y >= getY() && event.y < getY() + getHeight()) {
            // Handle click
            onClick();
        }
    }
    
    void paint(Display* display, Window window, GC gc) override {
        // Draw button background
        XSetForeground(display, gc, 0xCCCCCC); // Light gray
        XFillRectangle(display, window, gc, getX(), getY(), getWidth(), getHeight());
        
        // Draw button border
        XSetForeground(display, gc, 0x000000); // Black
        XDrawRectangle(display, window, gc, getX(), getY(), getWidth(), getHeight());
        
        // Draw button text
        XDrawString(display, window, gc, 
                    getX() + 10, getY() + getHeight() / 2 + 5, 
                    label.c_str(), label.length());
    }
    
    std::function<void()> onClick;

    std::string getLabel() const { return label; }
};