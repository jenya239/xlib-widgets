#include <memory>
#include <iostream>
#include <string>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>
#include <fstream>   // For std::ifstream
#include <iterator>  // For std::istreambuf_iterator
#include <bits/stl_algo.h>
#include <bits/stl_vector.h>

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
import ui.video_player.video_player;
import ui.image;
import ui.event;
import ui.event_listener;

bool isImageFile(const std::string& filePath) {
    // Check file extension
    std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // List of common image extensions
    const std::vector<std::string> imageExtensions = {
        "jpg", "jpeg", "png", "gif", "bmp", "tiff", "tif", "webp"
    };

    return std::find(imageExtensions.begin(), imageExtensions.end(), extension) != imageExtensions.end();
}

// Add this function to check if a file is a video file
bool isVideoFile(const std::string& filePath) {
    // Check file extension
    std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // List of common video extensions
    const std::vector<std::string> videoExtensions = {
        "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", "mpg", "mpeg"
    };

    return std::find(videoExtensions.begin(), videoExtensions.end(), extension) != videoExtensions.end();
}

int main() {
  	std::shared_ptr<Image> imageWidget;

    try {
        std::cout << "Starting application..." << std::endl;

        // Create application instance
        auto app = std::make_shared<Application>();

        // Create main window
        if (!app->createMainWindow("file browser", 900, 900)) {
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

        // Create a video player widget
        auto videoPlayer = std::make_shared<VideoPlayer>("video1", 50, 350, 250, 150, "/path/to/your/video.mp4");
        videoPlayer->setVisible(true);
        videoPlayer->markDirty();

        // Create a file browser
        auto fileBrowser = std::make_shared<FileBrowser>(320, 50, 450, 370, "/home");
        fileBrowser->setVisible(true);
        fileBrowser->setFocus(true);
        fileBrowser->markDirty();

        try {
        	imageWidget = std::make_shared<Image>("image1", 500, 500, 200, 150,
            	"/home/jenya/Pictures/jenya239_greyhounds_nebula_starlight_dc31de9a-2bc0-474d-b783-f762dc48bd25.png");
        	imageWidget->setVisible(true);
        	imageWidget->markDirty();
        	mainWindow->addChild(imageWidget);
        	logger->info("Image widget added successfully");
    	} catch (const std::exception& e) {
        	logger->error("Failed to create image widget: " + std::string(e.what()));
    	}

getFileSelectedSignal()->connect([logger, textArea, imageWidget, videoPlayer](const auto& payload) {
    // First check if it's a directory - if so, we don't need to do anything with the image or text area
    if (payload.isDirectory) {
        logger->info("Directory selected: " + payload.filePath);
        return; // Early return for directories
    }

    // Now handle files
    if (isImageFile(payload.filePath)) {
        logger->info("Image file selected: " + payload.filePath);

        // Update the image widget with the new image
        if (imageWidget) {
            try {
                imageWidget->setImage(payload.filePath);
                logger->info("Image updated successfully");
            } catch (const std::exception& e) {
                logger->error("Failed to update image: " + std::string(e.what()));
            }
        }
    }
    else if (isVideoFile(payload.filePath)) {
        logger->info("Video file selected: " + payload.filePath);

        // Update the video player with the new video
        if (videoPlayer) {
            try {
                videoPlayer->setVideoPath(payload.filePath);
                logger->info("Video updated successfully");
            } catch (const std::exception& e) {
                logger->error("Failed to update video: " + std::string(e.what()));
            }
        }
    }
    else {
        std::string filePath = payload.filePath;
        logger->info("Text file selected: " + filePath);

        // Try to load file content into text area if it's not too large
        try {
            // Check file size first
            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                logger->error("Failed to open file: " + filePath);
        		textArea->setText("Error: Could not open file");
        		textArea->markDirty();
                return;
            }

            // Get file size
            const size_t fileSize = file.tellg();

            // Only load if file is smaller than 100KB
            const size_t maxFileSize = 100 * 1024;
            if (fileSize > maxFileSize) {
                logger->info("File too large to display: " + filePath +
                            " (" + std::to_string(fileSize) + " bytes)");
                textArea->setText("File too large to display" + filePath);
                textArea->markDirty();
                return;
            }

            // Reset file pointer to beginning
            file.seekg(0, std::ios::beg);

    		// Read file content with a fixed buffer size
    		std::string content;
    		content.reserve(static_cast<size_t>(fileSize) + 1); // Pre-allocate memory

    		const size_t bufferSize = 4096;
    		char buffer[bufferSize];

    		while (file) {
        		file.read(buffer, bufferSize);
        		content.append(buffer, file.gcount());
    		}

    		// Set text area content
    		textArea->setText(content);
    		textArea->markDirty();
            logger->info("Loaded file content: " + std::to_string(content.size()) + " bytes");
        } catch (const std::bad_alloc& e) {
            logger->error("Memory allocation failed when loading file: " + std::string(e.what()));
            textArea->setText("Error: Out of memory when loading file");
            textArea->markDirty();
        } catch (const std::exception& e) {
            logger->error("Error loading file: " + std::string(e.what()));
            textArea->setText("Error loading file: " + std::string(e.what()));
            textArea->markDirty();
        }
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
                videoPlayer->setFont(font);
                logger->info("Font set for TextField");
            } else {
                logger->error("Failed to load font for TextField");
            }
        }

        // Add widgets to main window
        mainWindow->addChild(button);
        mainWindow->addChild(textField);
        mainWindow->addChild(textArea);
        mainWindow->addChild(videoPlayer);
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