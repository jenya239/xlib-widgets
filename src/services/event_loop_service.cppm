// src/services/event_loop_service.cppm
module;
#include <memory>
#include <X11/Xlib.h>
#include <functional>
#include <unordered_map>
#include <string>
#include <iostream>

import services.xdisplay_service;
import ui.event;
import services.logger_service;

export module services.event_loop_service;

export class EventLoopService {
private:
    std::shared_ptr<XDisplayService> displayService;
    std::shared_ptr<LoggerService> logger; 
    bool running = false;
    std::unordered_map<Window, std::function<void(const Event&)>> eventHandlers;

public:
    explicit EventLoopService(std::shared_ptr<XDisplayService> displayService, 
                              std::shared_ptr<LoggerService> logger)
        : displayService(std::move(displayService)), logger(logger) {}

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
        switch (xEvent.type) {
            case ButtonPress:
                std::cout << "[DEBUG] ButtonPress event detected" << std::endl;
            if (xEvent.xbutton.button == Button1) {
                std::cout << "[DEBUG] Left mouse button pressed" << std::endl;
                return Event(EventType::MouseDown, xEvent.xbutton.x, xEvent.xbutton.y);
            }
            // Добавляем возврат для других кнопок мыши
            return Event(EventType::Unknown, xEvent.xbutton.x, xEvent.xbutton.y);

            case ButtonRelease:
                std::cout << "[DEBUG] ButtonRelease event detected" << std::endl;
            if (xEvent.xbutton.button == Button1) {
                std::cout << "[DEBUG] Left mouse button released" << std::endl;
                return Event(EventType::MouseUp, xEvent.xbutton.x, xEvent.xbutton.y);
            }
            // Добавляем возврат для других кнопок мыши
            return Event(EventType::Unknown, xEvent.xbutton.x, xEvent.xbutton.y);

            case KeyPress:
                std::cout << "[DEBUG] KeyPress event detected" << std::endl;
            return Event(EventType::KeyPressEvent, xEvent.xkey.x, xEvent.xkey.y);

            case Expose:
                std::cout << "[DEBUG] Expose event detected" << std::endl;
            return Event(EventType::PaintEvent, xEvent.xexpose.x, xEvent.xexpose.y);

            case MotionNotify:
//                std::cout << "[DEBUG] Mouse motion event detected" << std::endl;
            return Event(EventType::MouseMove, xEvent.xmotion.x, xEvent.xmotion.y);

            case EnterNotify:
                std::cout << "[DEBUG] Mouse entered window" << std::endl;
            return Event(EventType::MouseEnter, xEvent.xcrossing.x, xEvent.xcrossing.y);

            case LeaveNotify:
                std::cout << "[DEBUG] Mouse left window" << std::endl;
            return Event(EventType::MouseLeave, xEvent.xcrossing.x, xEvent.xcrossing.y);

            default:
                std::cout << "[DEBUG] Unknown event detected" << std::endl;
            return Event(EventType::Unknown);
        }
    }

};
