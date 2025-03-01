module;
#include <vector>
#include <memory>
#include <X11/Xlib.h>

import ui.event;
import ui.event_listener;

export module ui.widget;

export class Widget : public EventListener {
public:
    virtual void handleEvent(const Event& event) override {
        for (auto& child : children) {
            child->handleEvent(event);
        }
    }

    void addChild(std::shared_ptr<Widget> child) {
        children.push_back(std::move(child));
    }

    void setPosition(int x, int y) {
        this->x = x;
        this->y = y;
    }

    void setSize(unsigned int width, unsigned int height) {
        this->width = width;
        this->height = height;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

    virtual void paint(Display* display, Window window, GC gc) {
        // Default implementation - just paint children
        for (auto& child : children) {
            child->paint(display, window, gc);
        }
    }

private:
    std::vector<std::shared_ptr<Widget>> children;
    int x = 0;
    int y = 0;
    unsigned int width = 100;
    unsigned int height = 30;
};