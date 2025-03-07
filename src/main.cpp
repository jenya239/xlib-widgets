#include <memory>
#include <iostream>
#include <string>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>

// Import necessary modules
import core.ioc_container;
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import services.render_service;
import state.app_signals;
import ui.application;
import ui.button;
import ui.text_field;
import ui.text_area;
import ui.file_browser;
import ui.event;
import ui.event_listener;

int main() {
    try {
        std::cout << "Starting application..." << std::endl;

        // Create application instance
        auto app = std::make_shared<Application>();

        // Create main window
        if (!app->createMainWindow("file browser", 900, 500)) {
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

        // Create a text field with more visible properties
        auto textField = std::make_shared<TextField>(50, 120, 250, 30, "Type here...");
        textField->setVisible(true);  // Ensure visibility is set

        // Make sure the text field needs repainting
        textField->markDirty();

        // Create a text area for multi-line text input
        auto textArea = std::make_shared<TextArea>(50, 170, 250, 150, "Enter multi-line text here...");
        textArea->setVisible(true);
        textArea->markDirty();

        // Create a file browser
        auto fileBrowser = std::make_shared<FileBrowser>(320, 50, 450, 370, "/home");
        fileBrowser->setVisible(true);
        fileBrowser->markDirty();

        getFileSelectedSignal()->connect([logger](const auto& payload) {
            if (!payload.isDirectory) {
//              loadFile(payload.filePath);
                logger->info("file selected " + payload.filePath);
            }
        });

        // Load and set font for the text field
        if (displayService) {
            XftFont* font = XftFontOpenName(
                displayService,
                DefaultScreen(displayService),
                "Monospace-10"
            );

            if (font) {
                textField->setFont(font);
                textArea->setFont(font);
                fileBrowser->setFont(font);
                logger->info("Font set for TextField");
            } else {
                logger->error("Failed to load font for TextField");
            }
        }

        // Add widgets to main window
        mainWindow->addChild(button);
        mainWindow->addChild(textField);
        mainWindow->addChild(textArea);
        mainWindow->addChild(fileBrowser);

        // Create a custom event listener for text field focus
        class TextFieldEventListener : public EventListener {
        private:
            std::shared_ptr<TextField> textField;
            std::shared_ptr<LoggerService> logger;

        public:
            TextFieldEventListener(std::shared_ptr<TextField> tf, std::shared_ptr<LoggerService> log)
                : textField(tf), logger(log) {}

            void handleEvent(const Event& event) override {
                // Get event type using the appropriate method
                Event::Type eventType = event.getType();

                // Handle mouse button press for focus management
                if (eventType == Event::Type::MouseDown) {
                    // Get mouse coordinates from the event
                    int x = event.getX();
                    int y = event.getY();

                    // Check if click is within text field bounds
                    if (x >= textField->getX() && x <= textField->getX() + textField->getWidth() &&
                        y >= textField->getY() && y <= textField->getY() + textField->getHeight()) {
                        textField->setFocus(true);
                        logger->info("TextField focused");
                    } else if (textField->isFocused()) {
                        textField->setFocus(false);
                        logger->info("TextField lost focus");
                    }
                }
                // Handle key press events when text field is focused
                else if (eventType == Event::Type::KeyDown && textField->isFocused()) {
                    // Get key information from the event
                    char key = event.getKeycode();
//                    textField->handleKeyInput(key);
                    logger->info("Key pressed in TextField: " + std::string(1, key));
                }
            }
        };

//        // Create the event listener
//        auto textFieldListener = std::make_shared<TextFieldEventListener>(textField, logger);
//
//        // Register the event listener with the main window
//        mainWindow->addEventListener(textFieldListener);

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