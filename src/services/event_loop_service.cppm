module;
#include <X11/Xlib.h>
#include <functional>
#include <map>
#include <memory>
#include <chrono>
#include <thread>
#include <iostream>

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

                // Отладочный вывод для событий мыши
                if (xEvent.type == ButtonPress) {
                    std::cout << "X11 ButtonPress event received: button=" << xEvent.xbutton.button << std::endl;
                } else if (xEvent.type == ButtonRelease) {
                    std::cout << "X11 ButtonRelease event received: button=" << xEvent.xbutton.button << std::endl;
                }

                // Convert X11 event to our custom Event
                Event event(xEvent);

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
};