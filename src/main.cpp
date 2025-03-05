#include <memory>
#include <iostream>
#include <string>
#include <X11/Xft/Xft.h>

// Import necessary modules
import core.ioc_container;
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import services.render_service;
import ui.application;
import ui.button;
import ui.text_field;

int main() {
    try {
        std::cout << "Starting application..." << std::endl;

        // Create application instance
        auto app = std::make_shared<Application>();

        // Create main window
        if (!app->createMainWindow("Simple Test Application", 600, 400)) {
            std::cerr << "Failed to create main window" << std::endl;
            return 1;
        }

        auto logger = app->getLogger();
        logger->info("Application initialized");

        auto mainWindow = app->getMainWindow();
        auto displayService = app->getMainWindow()->getDisplay();

        // Create a button
        auto button = std::make_shared<Button>("Test Button");
        button->setPosition(50, 50);
        button->setSize(150, 40);

        // Set button event handlers
        button->setOnClick([&logger]() {
            logger->info("Button clicked!");
        });

        button->setOnMouseEnter([&logger]() {
            logger->info("Mouse entered button");
        });

        button->setOnMouseLeave([&logger]() {
            logger->info("Mouse left button");
        });

        // Create a text field
        auto textField = std::make_shared<TextField>(50, 120, 250, 30, "Type here...");

        // Load and set font for the text field
        if (displayService) {
            XftFont* font = XftFontOpenName(
                displayService,
                DefaultScreen(displayService),
                "Sans-12"
            );

            if (font) {
                textField->setFont(font);
                logger->info("Font set for TextField");
            } else {
                logger->error("Failed to load font for TextField");
            }
        }

        // Add widgets to main window
        mainWindow->addChild(button);
        mainWindow->addChild(textField);

        // Setup event handling
        app->setupEventHandling();

        // Initial render
        app->initialRender();

        logger->info("Starting event loop");

        // Run the application
        app->run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}