// src/main.cpp
#include <memory>
#include <iostream>
#include <X11/Xlib.h>

import ui.widget;
import ui.event;
import ui.button;  // Add this import
import core.ioc_container;
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;

// Create a custom button with specific behavior
class MyButton : public Button {
private:
    std::shared_ptr<LoggerService> logger;

public:
    MyButton(std::string label, std::shared_ptr<LoggerService> logger) 
        : Button(std::move(label)), logger(logger) {

             // Определяем поведение для onClick
            onClick = [this, logger]() {
                logger->info("Button clicked: " + getLabel());
            };
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
    
    // Create graphics context
    GC gc = displayService->createGC(window);
    
    // Create UI hierarchy
    auto root = std::make_shared<Widget>();
    
    // Create a button and position it
    auto button = std::make_shared<MyButton>("Click Me", loggerService);
    button->setPosition(50, 50);
    button->setSize(100, 40);
    root->addChild(button);
    
    // Create another button
    auto exitButton = std::make_shared<MyButton>("Exit", loggerService);
    exitButton->setPosition(50, 120);
    exitButton->setSize(100, 40);
    exitButton->onClick = [&eventLoopService]() {
        eventLoopService->stop();
    };
    root->addChild(exitButton);
    
    // Register event handler for the window
    eventLoopService->registerEventHandler(window, [root, displayService, window](const Event& event) {
        if (event.type == "paint") {
            // Handle paint events
            Display* display = displayService->getDisplay();
            GC gc = displayService->createGC(window);
            root->paint(display, window, gc);
        } else {
            // Handle other events
            root->handleEvent(event);
        }
    });
    
    // Select input events for the window
    XSelectInput(displayService->getDisplay(), window, 
                 ExposureMask | ButtonPressMask | KeyPressMask);
    
    // Flush pending operations
    displayService->flush();
    
    loggerService->info("Starting event loop...");
    eventLoopService->start();
    
    loggerService->info("Application shutting down");
    
    return 0;
}