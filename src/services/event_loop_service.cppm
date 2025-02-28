// src/services/event_loop_service.cppm
module;
#include <memory>
#include <X11/Xlib.h>
#include <functional>
#include <unordered_map>
#include <string>

import services.xdisplay_service;
import ui.event;

export module services.event_loop_service;

export class EventLoopService {
private:
    std::shared_ptr<XDisplayService> displayService;
    bool running = false;
    std::unordered_map<Window, std::function<void(const Event&)>> eventHandlers;

public:
    explicit EventLoopService(std::shared_ptr<XDisplayService> displayService)
        : displayService(std::move(displayService)) {}

    void registerEventHandler(Window window, std::function<void(const Event&)> handler) {
        eventHandlers[window] = std::move(handler);
    }

    void start() {
        running = true;
        XEvent xEvent;
        Display* display = displayService->getDisplay();

        while (running) {
            XNextEvent(display, &xEvent);
            
            // Convert XEvent to our custom Event type
            Event event = convertXEventToEvent(xEvent);
            
            // Find handler for this window
            auto it = eventHandlers.find(xEvent.xany.window);
            if (it != eventHandlers.end()) {
                it->second(event);
            }
        }
    }

    void stop() {
        running = false;
    }

private:
    Event convertXEventToEvent(const XEvent& xEvent) {
        // Simple conversion from XEvent to our custom Event
        switch (xEvent.type) {
            case ButtonPress:
                return Event("click");
            case KeyPress:
                return Event("keypress");
            case Expose:
                return Event("paint");
            default:
                return Event("unknown");
        }
    }
};
