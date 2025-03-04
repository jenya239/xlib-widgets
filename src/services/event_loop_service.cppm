module;
#include <X11/Xlib.h>
#include <functional>
#include <map>
#include <memory>
#include <chrono>
#include <thread>

export module services.event_loop_service;

import services.xdisplay_service;
import services.logger_service;
import ui.event;

export class EventLoopService {
private:
    XDisplayService& displayService;
    LoggerService& logger;
    std::map<Window, std::function<void(const Event&)>> eventHandlers;
    std::function<void()> redrawCallback;
    bool running = false;
    std::chrono::milliseconds redrawInterval{16}; // ~60 FPS

public:
    EventLoopService(XDisplayService& displayService, LoggerService& logger)
        : displayService(displayService), logger(logger) {}

    // Register an event handler for a specific window
    void registerEventHandler(Window window, std::function<void(const Event&)> handler) {
        eventHandlers[window] = std::move(handler);
    }

    // Set a callback for periodic redrawing
    void setRedrawCallback(std::function<void()> callback) {
        redrawCallback = std::move(callback);
    }

    // Run the event loop
    void run() {
        running = true;
        Display* display = displayService.getDisplay();

        auto lastRedrawTime = std::chrono::steady_clock::now();

        while (running && display) {
            // Process all pending events
            while (XPending(display) > 0) {
                XEvent xEvent;
                XNextEvent(display, &xEvent);

                // Convert X11 event to our custom Event
                Event event = convertXEvent(xEvent);

                // Find and call the appropriate handler
                auto it = eventHandlers.find(xEvent.xany.window);
                if (it != eventHandlers.end()) {
                    it->second(event);
                }
            }

            // Check if it's time for a redraw
            auto currentTime = std::chrono::steady_clock::now();
            if (redrawCallback &&
                (currentTime - lastRedrawTime >= redrawInterval)) {
                redrawCallback();
                lastRedrawTime = currentTime;
            }

            // Small sleep to prevent CPU hogging
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Stop the event loop
    void stop() {
        running = false;
    }

private:
    // Convert X11 event to our custom Event
    Event convertXEvent(const XEvent& xEvent) {
        Event::Type type;
        int x = 0, y = 0;

        switch (xEvent.type) {
            case Expose:
                type = Event::Type::PaintEvent;
                break;
            case ButtonPress:
                type = Event::Type::MouseDown;
                x = xEvent.xbutton.x;
                y = xEvent.xbutton.y;
                break;
            case ButtonRelease:
                type = Event::Type::MouseUp;
                x = xEvent.xbutton.x;
                y = xEvent.xbutton.y;
                break;
            case MotionNotify:
                type = Event::Type::MouseMove;
                x = xEvent.xmotion.x;
                y = xEvent.xmotion.y;
                break;
            case KeyPress:
                type = Event::Type::KeyDown;
                break;
            case KeyRelease:
                type = Event::Type::KeyUp;
                break;
            default:
                type = Event::Type::Unknown;
                break;
        }

        return Event(type, x, y);
    }
};