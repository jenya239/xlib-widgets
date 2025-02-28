// src/main.cpp
#include <memory>
#include <iostream>
#include <X11/Xlib.h>

import ui.widget;
import ui.event;
import core.ioc_container;
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;

class Button : public Widget {
public:
    void handleEvent(const Event& event) override {
        if (event.type == "click") {
            std::cout << "Button clicked!\n";
        }
    }
};

int main() {
    // Initialize IoC container
    auto& ioc = IoC::getInstance();
    
    // Register XDisplayService
    auto displayService = std::make_shared<XDisplayService>();
    ioc.registerSingleton<XDisplayService>(displayService);
    
    // Register EventLoopService
    auto eventLoopService = std::make_shared<EventLoopService>(displayService);
    ioc.registerSingleton<EventLoopService>(eventLoopService);

    // Register LoggerService
    auto loggerService = std::make_shared<LoggerService>();
    ioc.registerSingleton<LoggerService>(loggerService);

    // Log some information
    loggerService->info("Application starting up");
    loggerService->debug("Display service initialized");
    
    // Create a window
    Window window = displayService->createWindow(100, 100, 400, 300);
    displayService->mapWindow(window);
    
    // Create UI hierarchy
    auto root = std::make_shared<Widget>();
    auto button = std::make_shared<Button>();
    root->addChild(button);
    
    // Register event handler for the window
    eventLoopService->registerEventHandler(window, [root](const Event& event) {
        root->handleEvent(event);
    });
    
    // Select input events for the window
    XSelectInput(displayService->getDisplay(), window, ExposureMask | ButtonPressMask | KeyPressMask);
    
    // Flush pending operations
    displayService->flush();
    
    std::cout << "Starting event loop...\n";
    eventLoopService->start();
    
    return 0;
}
