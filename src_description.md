# Исходные файлы проекта

## Краткая статистика

- Всего файлов: 19
- Типы файлов: .cppm, .cpp

## Структура директорий

```
src/
  └── main.cpp
src/core/
  └── ioc_container.cppm
src/services/
  └── event_loop_service.cppm
  └── logger_service.cppm
  └── render_service.cppm
  └── xdisplay_service.cppm
src/state/
  └── app_signals.cppm
  └── signal.cppm
src/ui/
  └── application.cppm
  └── application_window.cppm
  └── button.cppm
  └── event.cppm
  └── event_listener.cppm
  └── file_browser.cppm
  └── image.cppm
  └── render_buffer.cppm
  └── text_area.cppm
  └── text_field.cppm
  └── widget.cppm
```

## Содержимое файлов

### Файл: `src/core/ioc_container.cppm`

```cpp
module;
#include <unordered_map>
#include <memory>
#include <string>
#include <typeinfo>
#include <functional>
#include <stdexcept>

export module core.ioc_container;

export class IoC {
private:
    struct TypeInfoHash {
        std::size_t operator()(const std::type_info* ti) const {
            return ti->hash_code();
        }
    };

    struct TypeInfoEqual {
        bool operator()(const std::type_info* lhs, const std::type_info* rhs) const {
            return *lhs == *rhs;
        }
    };

    std::unordered_map<const std::type_info*, std::shared_ptr<void>, TypeInfoHash, TypeInfoEqual> instances;
    std::unordered_map<const std::type_info*, std::function<std::shared_ptr<void>()>, TypeInfoHash, TypeInfoEqual> factories;

public:
    // Register a singleton instance
    template<typename T>
    void registerSingleton(std::shared_ptr<T> instance) {
        instances[&typeid(T)] = std::static_pointer_cast<void>(instance);
    }

    // Register a factory for creating instances
    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>()> factory) {
        factories[&typeid(T)] = [factory]() {
            return std::static_pointer_cast<void>(factory());
        };
    }

    // Get or create an instance
    template<typename T>
    std::shared_ptr<T> resolve() {
        const std::type_info* typeId = &typeid(T);
        
        // Check if we have a singleton instance
        auto it = instances.find(typeId);
        if (it != instances.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        
        // Check if we have a factory
        auto factoryIt = factories.find(typeId);
        if (factoryIt != factories.end()) {
            auto instance = std::static_pointer_cast<T>(factoryIt->second());
            return instance;
        }
        
        throw std::runtime_error("Type not registered: " + std::string(typeId->name()));
    }

    // Singleton pattern for the container itself
    static IoC& getInstance() {
        static IoC instance;
        return instance;
    }
};
```

---

### Файл: `src/main.cpp`

```cpp
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

        // Create a file browser
        auto fileBrowser = std::make_shared<FileBrowser>(320, 50, 450, 370, "/home");
        fileBrowser->setVisible(true);
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

getFileSelectedSignal()->connect([logger, textArea, imageWidget](const auto& payload) {
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
    } else {
        logger->info("Text file selected: " + payload.filePath);

        // Try to load file content into text area if it's not too large
        try {
            // Check file size first
            std::ifstream file(payload.filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                logger->error("Failed to open file: " + payload.filePath);
                return;
            }

            // Get file size
            const size_t fileSize = file.tellg();

            // Only load if file is smaller than 100KB
            const size_t maxFileSize = 100 * 1024;
            if (fileSize > maxFileSize) {
                logger->info("File too large to display: " + payload.filePath +
                            " (" + std::to_string(fileSize) + " bytes)");
                textArea->setText("File too large to display");
                return;
            }

            // Reset file pointer to beginning
            file.seekg(0, std::ios::beg);

            // Read the file content
            std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

            // Set text area content
            textArea->setText(content);
            textArea->markDirty();
            logger->info("Loaded file content: " + std::to_string(content.size()) + " bytes");
        } catch (const std::bad_alloc& e) {
            logger->error("Memory allocation failed when loading file: " + std::string(e.what()));
            textArea->setText("Error: Out of memory when loading file");
        } catch (const std::exception& e) {
            logger->error("Error loading file: " + std::string(e.what()));
            textArea->setText("Error loading file: " + std::string(e.what()));
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
```

---

### Файл: `src/services/event_loop_service.cppm`

```cpp
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
```

---

### Файл: `src/services/logger_service.cppm`

```cpp
module;
#include <iostream>
#include <string>
#include <memory>

export module services.logger_service;

export enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

export class LoggerService {
private:
    LogLevel currentLevel = LogLevel::DEBUG;

public:
    void setLogLevel(LogLevel level) {
        currentLevel = level;
    }

    void debug(const std::string& message) {
        if (currentLevel <= LogLevel::DEBUG) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }

    void info(const std::string& message) {
        if (currentLevel <= LogLevel::INFO) {
            std::cout << "[INFO] " << message << std::endl;
        }
    }

    void warning(const std::string& message) {
        if (currentLevel <= LogLevel::WARNING) {
            std::cout << "[WARNING] " << message << std::endl;
        }
    }

    void error(const std::string& message) {
        if (currentLevel <= LogLevel::ERROR) {
            std::cout << "[ERROR] " << message << std::endl;
        }
    }
};
```

---

### Файл: `src/services/render_service.cppm`

```cpp
module;
#include <X11/Xlib.h>
#include <iostream>
#include <vector>

export module services.render_service;

import ui.widget;
import ui.render_buffer;

export class RenderService {
public:
	// Repaint all dirty widgets
	static void repaintDirtyWidgets(Display* display, Window window, GC gc, Widget* rootWidget) {
		if (!rootWidget) return;

		// First render to buffers
		rootWidget->render(display, window);

		// Then paint to window
		rootWidget->paint(display, window, gc);
	}

	// Find all dirty widgets in the hierarchy
	static std::vector<Widget*> findDirtyWidgets(Widget* rootWidget) {
		std::vector<Widget*> dirtyWidgets;
		collectDirtyWidgets(rootWidget, dirtyWidgets);
		return dirtyWidgets;
	}

private:
	// Helper method to collect dirty widgets
	static void collectDirtyWidgets(Widget* widget, std::vector<Widget*>& dirtyWidgets) {
		if (!widget) return;

		if (widget->needsRepaint()) {
			dirtyWidgets.push_back(widget);
		}

		// Check children
		for (auto* child : widget->getChildren()) {
			collectDirtyWidgets(child, dirtyWidgets);
		}
	}
};
```

---

### Файл: `src/services/xdisplay_service.cppm`

```cpp
// src/services/xdisplay_service.cppm
module;
#include <memory>
#include <X11/Xlib.h>
#include <stdexcept>
#include <string>

export module services.xdisplay_service;

export class XDisplayService {
private:
    Display* display = nullptr;

public:
    XDisplayService() {
        display = XOpenDisplay(nullptr);
        if (!display) {
            throw std::runtime_error("Failed to open X display");
        }
    }

    ~XDisplayService() {
        if (display) {
            XCloseDisplay(display);
            display = nullptr;
        }
    }

    // Prevent copying
    XDisplayService(const XDisplayService&) = delete;
    XDisplayService& operator=(const XDisplayService&) = delete;

    // Allow moving
    XDisplayService(XDisplayService&& other) noexcept {
        display = other.display;
        other.display = nullptr;
    }

    XDisplayService& operator=(XDisplayService&& other) noexcept {
        if (this != &other) {
            if (display) {
                XCloseDisplay(display);
            }
            display = other.display;
            other.display = nullptr;
        }
        return *this;
    }

    Display* getDisplay() const {
        return display;
    }

    // Create a simple window
    Window createWindow(int x, int y, unsigned int width, unsigned int height) {
        int screen = DefaultScreen(display);
        Window root = RootWindow(display, screen);
        unsigned long black = BlackPixel(display, screen);
        unsigned long white = WhitePixel(display, screen);

        Window window = XCreateSimpleWindow(
            display, root, x, y, width, height, 1, black, white
        );

        return window;
    }

    // Map window to screen
    void mapWindow(Window window) {
        XMapWindow(display, window);
    }

    // Flush display
    void flush() {
        XFlush(display);
    }

    // Create a graphics context
    GC createGC(Window window) {
        return XCreateGC(display, window, 0, nullptr);
    }

    // Draw rectangle
    void drawRectangle(Window window, GC gc, int x, int y, unsigned int width, unsigned int height) {
        XDrawRectangle(display, window, gc, x, y, width, height);
    }

    // Fill rectangle
    void fillRectangle(Window window, GC gc, int x, int y, unsigned int width, unsigned int height) {
        XFillRectangle(display, window, gc, x, y, width, height);
    }

    // Set foreground color
    void setForeground(GC gc, unsigned long color) {
        XSetForeground(display, gc, color);
    }

    // Draw text
    void drawText(Window window, GC gc, int x, int y, const std::string& text) {
        XDrawString(display, window, gc, x, y, text.c_str(), text.length());
    }
};

```

---

### Файл: `src/state/app_signals.cppm`

```cpp
module;
#include <string>
#include <vector>
#include <memory>

export module state.app_signals;

import state.signal;

// Типы данных для сигналов
export struct FileSelectedPayload {
	std::string filePath;
	std::string fileName;
	bool isDirectory;
};

export struct TextChangedPayload {
	std::string text;
	std::string widgetId;
};

export struct ButtonClickedPayload {
	std::string buttonId;
};

// Функции-обёртки для получения сигналов (синглтоны)
export std::shared_ptr<Signal<FileSelectedPayload>> getFileSelectedSignal() {
	static std::shared_ptr<Signal<FileSelectedPayload>> signal =
			std::make_shared<Signal<FileSelectedPayload>>();
	static bool initialized = false;
	if (!initialized) {
		SignalService::registerSignal(signal);
		initialized = true;
	}
	return signal;
}

export std::shared_ptr<Signal<TextChangedPayload>> getTextChangedSignal() {
	static std::shared_ptr<Signal<TextChangedPayload>> signal =
			std::make_shared<Signal<TextChangedPayload>>();
	static bool initialized = false;
	if (!initialized) {
		SignalService::registerSignal(signal);
		initialized = true;
	}
	return signal;
}

export std::shared_ptr<Signal<ButtonClickedPayload>> getButtonClickedSignal() {
	static std::shared_ptr<Signal<ButtonClickedPayload>> signal =
			std::make_shared<Signal<ButtonClickedPayload>>();
	static bool initialized = false;
	if (!initialized) {
		SignalService::registerSignal(signal);
		initialized = true;
	}
	return signal;
}
```

---

### Файл: `src/state/signal.cppm`

```cpp
module;
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <any>

export module state.signal;

// Уникальный идентификатор сигнала
export class SignalId {
private:
    // Приватный конструктор гарантирует, что каждый экземпляр уникален
    SignalId() = default;

public:
    // Фабричный метод для создания уникальных идентификаторов
    static std::shared_ptr<SignalId> create() {
        return std::shared_ptr<SignalId>(new SignalId());
    }

    // Запрещаем копирование и перемещение
    SignalId(const SignalId&) = delete;
    SignalId& operator=(const SignalId&) = delete;
    SignalId(SignalId&&) = delete;
    SignalId& operator=(SignalId&&) = delete;
};

// Класс для создания и управления сигналами
export template<typename PayloadType>
class Signal {
private:
    using SignalHandler = std::function<void(const PayloadType&)>;
    std::vector<SignalHandler> handlers;
    std::shared_ptr<SignalId> id;

public:
    Signal() : id(SignalId::create()) {}

    // Получить уникальный идентификатор сигнала
    std::shared_ptr<SignalId> getId() const {
        return id;
    }

    // Отправить сигнал с данными
    void emit(const PayloadType& payload) const {
        for (const auto& handler : handlers) {
            handler(payload);
        }
    }

    // Подписаться на сигнал
    template<typename Handler>
    void connect(Handler&& handler) {
        handlers.push_back(std::forward<Handler>(handler));
    }
};

// Сервис для управления сигналами
export class SignalService {
private:
    // Хранилище для сигналов по их идентификаторам
    std::unordered_map<std::shared_ptr<SignalId>, std::any> signals;

    // Синглтон
    static SignalService& getInstance() {
        static SignalService instance;
        return instance;
    }

    SignalService() = default;

public:
    // Регистрация сигнала в сервисе
    template<typename PayloadType>
    static void registerSignal(const std::shared_ptr<Signal<PayloadType>>& signal) {
        auto& instance = getInstance();
        instance.signals[signal->getId()] = signal;
    }

    // Получить сигнал по идентификатору
    template<typename PayloadType>
    static std::shared_ptr<Signal<PayloadType>> getSignal(const std::shared_ptr<SignalId>& id) {
        auto& instance = getInstance();
        auto it = instance.signals.find(id);
        if (it != instance.signals.end()) {
            return std::any_cast<std::shared_ptr<Signal<PayloadType>>>(it->second);
        }
        return nullptr;
    }
};
```

---

### Файл: `src/ui/application.cppm`

```cpp
module;
#include <memory>
#include <string>
#include <X11/Xlib.h>

export module ui.application;

import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import services.render_service;  // Make sure this is properly imported
import ui.application_window;
import ui.button;
import ui.event;
import ui.widget;

export class Application {
private:
    std::shared_ptr<XDisplayService> displayService;
    std::shared_ptr<EventLoopService> eventLoop;
    std::shared_ptr<LoggerService> logger;
    std::shared_ptr<ApplicationWindow> mainWindow;

public:
    Application() {
        // Initialize services
        displayService = std::make_shared<XDisplayService>();
        // XDisplayService constructor already initializes the display
        // No need to call initialize() as it doesn't exist

        logger = std::make_shared<LoggerService>();

        // Pass references instead of shared_ptr objects
        eventLoop = std::make_shared<EventLoopService>(*displayService, *logger);
    }

    ~Application() {
        // Clean up resources
        // XDisplayService destructor already handles cleanup
        // No need to call cleanup() as it doesn't exist
    }

    bool createMainWindow(const std::string& title, int width, int height) {
        mainWindow = std::make_shared<ApplicationWindow>(title, width, height);
        return mainWindow->initialize(displayService->getDisplay());
    }

    std::shared_ptr<ApplicationWindow> getMainWindow() const {
        return mainWindow;
    }

    std::shared_ptr<LoggerService> getLogger() const {
        return logger;
    }

    void setupEventHandling() {
        if (!mainWindow) return;

        Display* display = displayService->getDisplay();
        Window xWindow = mainWindow->getNativeWindow();

        // Register event handler for the main window
        eventLoop->registerEventHandler(xWindow, [this](const Event& event) {
            if (mainWindow) {
                mainWindow->handleEvent(event);
            }
        });
    }

    void initialRender() {
        if (!mainWindow) return;

        Display* display = displayService->getDisplay();
        Window xWindow = mainWindow->getNativeWindow();
        GC gc = XCreateGC(display, xWindow, 0, nullptr);

        // Render all widgets
        RenderService::repaintDirtyWidgets(display, mainWindow->getNativeWindow(), gc, mainWindow.get());

        XFreeGC(display, gc);
        XFlush(display);
    }

    void run() {
        if (!eventLoop || !mainWindow) return;

        Display* display = displayService->getDisplay();
        Window xWindow = mainWindow->getNativeWindow();

        // Set up periodic redraw
        eventLoop->setRedrawCallback([this]() {
            if (!mainWindow) return;

            Display* display = displayService->getDisplay();
            Window xWindow = mainWindow->getNativeWindow();
            GC gc = XCreateGC(display, xWindow, 0, nullptr);

            // Redraw all widgets that need updating
            RenderService::repaintDirtyWidgets(display, xWindow, gc, mainWindow.get());

            XFreeGC(display, gc);
            XFlush(display);
        });

        // Initial render
        GC gc = XCreateGC(display, xWindow, 0, nullptr);
        RenderService::repaintDirtyWidgets(display, xWindow, gc, mainWindow.get());
        XFreeGC(display, gc);
        XFlush(display);

        // Run the event loop
        eventLoop->run();
    }
};
```

---

### Файл: `src/ui/application_window.cppm`

```cpp
// window.cppm -> application_window.cppm
module;
#include <memory>
#include <X11/Xlib.h>
#include <string>

import ui.widget;
import ui.event;
import ui.event_listener;
import ui.render_buffer;  // Import the new module

export module ui.application_window;

export class ApplicationWindow : public Widget {
private:
    Display* display = nullptr;
    Window xWindow = 0;  // Use X11's Window type
    std::string title = "Application Window";
    int width = 1200;
    int height = 600;

public:
    // Конструктор по умолчанию
    ApplicationWindow() = default;

    // Конструктор с параметрами
    ApplicationWindow(const std::string& title, int width, int height)
        : title(title), width(width), height(height) {}

    // Метод инициализации, который создает X11 окно
    bool initialize(Display* display) {
        if (!display) return false;

        this->display = display;

        // Получаем экран по умолчанию
        int screen = DefaultScreen(display);

        // Получаем цвета для окна
        unsigned long black = BlackPixel(display, screen);
        unsigned long white = WhitePixel(display, screen);

        // Создаем окно
        xWindow = XCreateSimpleWindow(
            display,                  // Display
            DefaultRootWindow(display), // Родительское окно
            0, 0,                     // Позиция (x, y)
            width, height,            // Размеры
            1,                        // Ширина границы
            black,                    // Цвет границы
            white                     // Цвет фона
        );

        if (!xWindow) return false;

        // Устанавливаем заголовок окна
        XStoreName(display, xWindow, title.c_str());

        // Выбираем события, которые хотим получать
        // В методе setupEventHandling или при создании окна
        XSelectInput(display, xWindow, ExposureMask | ButtonPressMask | ButtonReleaseMask |
                              PointerMotionMask | EnterWindowMask | LeaveWindowMask | KeyPressMask | KeyReleaseMask);

        // Отображаем окно
        XMapWindow(display, xWindow);
        XFlush(display);

        // Устанавливаем размеры виджета
        setSize(width, height);

        return true;
    }

    void setNativeWindow(Display* display, Window xWindow) {
        this->display = display;
        this->xWindow = xWindow;
    }

    Display* getDisplay() const { return display; }
    Window getNativeWindow() const { return xWindow; }

    // Для совместимости с кодом, который может использовать getWindow()
    Window getWindow() const { return xWindow; }

    void handleEvent(const Event& event) override {
        Widget::handleEvent(event);
        
        // Handle paint events
        if (event.getType() == Event::Type::PaintEvent) {
            repaint();
        }
    }
    
    // Trigger a full render and paint cycle
    void repaint() {
        if (!display || !xWindow) return;
        
        // First render everything to buffers
        render(display, xWindow);
        
        // Then composite the buffers to the window
        GC gc = XCreateGC(display, xWindow, 0, nullptr);
        paint(display, xWindow, gc);
        XFreeGC(display, gc);
        
        // Make sure changes are visible
        XFlush(display);
    }
};
```

---

### Файл: `src/ui/button.cppm`

```cpp
module;
#include <string>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <functional>
#include <iostream>
#include <algorithm>

import ui.widget;
import ui.event;
import ui.render_buffer;

export module ui.button;

// Define button states
export enum class ButtonState {
    Normal,
    Hover,
    Pressed,
    Disabled
};

export class Button : public Widget {
private:
    std::string label;
    ButtonState state = ButtonState::Normal;
    bool isMouseOver = false;
    bool isPressed = false;
    XftFont* font = nullptr;
    XftDraw* xftDraw = nullptr;

    // Цвета для разных состояний кнопки
    unsigned long normalColor = 0xCCCCCC;
    unsigned long hoverColor = 0xAAAAAA;
    unsigned long pressedColor = 0x777777;
    unsigned long disabledColor = 0xEEEEEE;

    // Xft цвета для текста
    XftColor textColor;
    XftColor disabledTextColor;

    // Callbacks для различных событий
    std::function<void()> onClick;
    std::function<void()> onMouseEnter;
    std::function<void()> onMouseLeave;
    std::function<void()> onMouseDown;
    std::function<void()> onMouseUp;

    // Проверяет, находится ли точка внутри кнопки
    bool isPointInside(int x, int y) const {
        int absX, absY;
        getAbsolutePosition(absX, absY);
        return x >= absX && x < absX + static_cast<int>(getWidth()) &&
               y >= absY && y < absY + static_cast<int>(getHeight());
    }

    // Инициализация шрифта и цветов
    void initResources(Display* display) {
        if (!font) {
            // Загружаем шрифт (можно настроить размер и имя шрифта)
            font = XftFontOpenName(display, DefaultScreen(display), "Sans-12");
            if (!font) {
                std::cerr << "Failed to load font" << std::endl;
                return;
            }
        }

        // Инициализация цветов
        XftColorAllocName(
            display,
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display)),
            "#000000",
            &textColor
        );

        XftColorAllocName(
            display,
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display)),
            "#777777",
            &disabledTextColor
        );
    }

    // Освобождение ресурсов
    void cleanupResources(Display* display) {
        if (font) {
            XftFontClose(display, font);
            font = nullptr;
        }

        if (xftDraw) {
            XftDrawDestroy(xftDraw);
            xftDraw = nullptr;
        }

        XftColorFree(
            display,
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display)),
            &textColor
        );

        XftColorFree(
            display,
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display)),
            &disabledTextColor
        );
    }

public:
    Button(const std::string& buttonLabel = "", const std::string& id = "")
        : Widget(id), label(buttonLabel) {}

    ~Button() {
        // Ресурсы будут освобождены в cleanupResources
    }

    // Установка текста кнопки
    void setLabel(const std::string& newLabel) {
        if (label != newLabel) {
            label = newLabel;
            markDirty();
        }
    }

    const std::string& getLabel() const {
        return label;
    }

    void setState(ButtonState newState) {
        if (state != newState) {
            std::cout << "Button state changed: " << static_cast<int>(state) << " -> " << static_cast<int>(newState) << std::endl;
            state = newState;
            markDirty();
        }
    }

    ButtonState getState() const {
        return state;
    }

    // Методы для установки обработчиков событий
    void setOnClick(std::function<void()> handler) {
        onClick = handler;
    }

    void setOnMouseEnter(std::function<void()> handler) {
        onMouseEnter = handler;
    }

    void setOnMouseLeave(std::function<void()> handler) {
        onMouseLeave = handler;
    }

    void setOnMouseDown(std::function<void()> handler) {
        onMouseDown = handler;
    }

    void setOnMouseUp(std::function<void()> handler) {
        onMouseUp = handler;
    }

    // Переопределение метода обработки событий
    void handleEvent(const Event& event) override {
        if (!isEnabled) return;

        // Отладочный вывод
//        std::cout << "Button::handleEvent called, event type: " << event.getNativeEvent().type << std::endl;

        // Получаем XEvent из Event
        Event::Type eventType = event.getType();

        // Обработка событий мыши
        if (eventType == Event::Type::MouseDown) {
            if (isPointInside(event.getX(), event.getY()) && event.getButton() == Button1) {
                std::cout << "MouseDown detected inside button" << std::endl;
                isPressed = true;
                setState(ButtonState::Pressed);
                if (onMouseDown) onMouseDown();
            }
        }
        else if (eventType == Event::Type::MouseUp) {
            if (isPressed && event.getButton() == Button1) {
                std::cout << "MouseUp detected, was pressed: " << isPressed << std::endl;
                isPressed = false;
                if (onMouseUp) onMouseUp();

                if (isPointInside(event.getX(), event.getY())) {
                    std::cout << "MouseUp inside button, triggering onClick" << std::endl;
                    if (onClick) onClick();
                    setState(isMouseOver ? ButtonState::Hover : ButtonState::Normal);
                } else {
                    setState(ButtonState::Normal);
                }
            }
        }
        else if (eventType == Event::Type::MouseMove) {
            bool wasMouseOver = isMouseOver;
            isMouseOver = isPointInside(event.getX(), event.getY());

            if (isMouseOver != wasMouseOver) {
                if (isMouseOver) {
                    setState(isPressed ? ButtonState::Pressed : ButtonState::Hover);
                    if (onMouseEnter) onMouseEnter();
                } else {
                    setState(ButtonState::Normal);
                    if (onMouseLeave) onMouseLeave();
                }
            }
        }
        else if (eventType == Event::Type::MouseEnter) {
            isMouseOver = true;
            setState(isPressed ? ButtonState::Pressed : ButtonState::Hover);
            if (onMouseEnter) onMouseEnter();
        }
        else if (eventType == Event::Type::MouseLeave) {
            isMouseOver = false;
            setState(ButtonState::Normal);
            if (onMouseLeave) onMouseLeave();
        }

        // Передаем событие дочерним виджетам
        Widget::handleEvent(event);
    }

    // Переопределение метода отрисовки
    void paintToBuffer(Display* display) override {
        ensureBuffer(display, getWindow());
        RenderBuffer* buffer = getBuffer();
        if (!buffer) return;

        // Инициализация ресурсов, если необходимо
        initResources(display);

        // Определение цвета фона в зависимости от состояния
        unsigned long bgColor;
        switch (state) {
            case ButtonState::Hover:
                bgColor = hoverColor;
            std::cout << "Painting button in HOVER state" << std::endl;
            break;
            case ButtonState::Pressed:
                bgColor = pressedColor;
            std::cout << "Painting button in PRESSED state" << std::endl;
            break;
            case ButtonState::Disabled:
                bgColor = disabledColor;
            std::cout << "Painting button in DISABLED state" << std::endl;
            break;
            default:
                bgColor = normalColor;
            std::cout << "Painting button in NORMAL state" << std::endl;
        }

        // Рисуем фон кнопки
        buffer->fillRectangle(0, 0, getWidth(), getHeight(), bgColor);

        // Рисуем рамку
        buffer->drawRectangle(0, 0, getWidth() - 1, getHeight() - 1, 0x000000);

        // Создаем XftDraw для рисования текста
        if (xftDraw) {
            XftDrawDestroy(xftDraw);
        }

        xftDraw = XftDrawCreate(
            display,
            buffer->getPixmap(),
            DefaultVisual(display, DefaultScreen(display)),
            DefaultColormap(display, DefaultScreen(display))
        );

        if (xftDraw && font) {
            // Вычисляем размеры текста
            XGlyphInfo extents;
            XftTextExtentsUtf8(
                display,
                font,
                reinterpret_cast<const FcChar8*>(label.c_str()),
                label.length(),
                &extents
            );

            // Центрируем текст
            int textX = (getWidth() - extents.width) / 2;
            int textY = (getHeight() + font->ascent - font->descent) / 2;

            // Выбираем цвет текста в зависимости от состояния
            XftColor* currentTextColor = (state == ButtonState::Disabled) ? &disabledTextColor : &textColor;

            // Рисуем текст
            XftDrawStringUtf8(
                xftDraw,
                currentTextColor,
                font,
                textX,
                textY,
                reinterpret_cast<const FcChar8*>(label.c_str()),
                label.length()
            );
        }
    }

    // Метод для освобождения ресурсов при уничтожении
    void cleanup(Display* display) {
        cleanupResources(display);
    }
};
```

---

### Файл: `src/ui/event.cppm`

```cpp
module;
#include <X11/Xlib.h>
#include <iostream>

export module ui.event;

export class Event {
public:
    // Переименуем в Type, чтобы избежать конфликтов с EventType
    enum class Type {
        MouseMove,
        MouseDown,
        MouseUp,
        MouseEnter,
        MouseLeave,
        KeyDown,
        KeyUp,
        KeyPressEvent,  // Добавлено для совместимости
        PaintEvent,     // Добавлено для совместимости
        WindowResize,
        WindowClose,
        Unknown
    };

private:
    Type type;
    int x;
    int y;
    unsigned int button;
    unsigned int keycode;
    XEvent nativeEvent;

public:
    Event(Type type = Type::Unknown, int x = 0, int y = 0,
          unsigned int button = 0, unsigned int keycode = 0)
        : type(type), x(x), y(y), button(button), keycode(keycode) {}

    Event(const XEvent& xEvent) : nativeEvent(xEvent) {
        // Преобразование XEvent в наш Event
        switch (xEvent.type) {
            case MotionNotify:
                type = Type::MouseMove;
                x = xEvent.xmotion.x;
                y = xEvent.xmotion.y;
                break;
            case ButtonPress:
                type = Event::Type::MouseDown;
                x = xEvent.xbutton.x;
                y = xEvent.xbutton.y;
                button = xEvent.xbutton.button;  // Сохраняем номер кнопки
                std::cout << "Converting ButtonPress to MouseDown, button=" << button << std::endl;
                break;
            case ButtonRelease:
                type = Event::Type::MouseUp;
                x = xEvent.xbutton.x;
                y = xEvent.xbutton.y;
                button = xEvent.xbutton.button;  // Сохраняем номер кнопки
                std::cout << "Converting ButtonRelease to MouseUp, button=" << button << std::endl;
                break;
            case EnterNotify:
                type = Type::MouseEnter;
                x = xEvent.xcrossing.x;
                y = xEvent.xcrossing.y;
                break;
            case LeaveNotify:
                type = Type::MouseLeave;
                x = xEvent.xcrossing.x;
                y = xEvent.xcrossing.y;
                break;
            case KeyPress:
                type = Type::KeyDown;
                keycode = xEvent.xkey.keycode;
                break;
            case KeyRelease:
                type = Type::KeyUp;
                keycode = xEvent.xkey.keycode;
                break;
            case ConfigureNotify:
                type = Type::WindowResize;
                break;
            case ClientMessage:
                // Проверка на сообщение о закрытии окна
                type = Type::WindowClose;
                break;
            default:
                type = Type::Unknown;
                break;
        }
    }

    // Геттеры для доступа к приватным полям
    Type getType() const { return type; }
    int getX() const { return x; }
    int getY() const { return y; }
    unsigned int getButton() const { return button; }
    unsigned int getKeycode() const { return keycode; }
    const XEvent& getNativeEvent() const { return nativeEvent; }

    // Метод для получения строкового представления типа события
    std::string getName() const {
        switch (type) {
            case Type::MouseMove:
                return "MouseMove";
            case Type::MouseDown:
                return "MouseDown";
            case Type::MouseUp:
                return "MouseUp";
            case Type::MouseEnter:
                return "MouseEnter";
            case Type::MouseLeave:
                return "MouseLeave";
            case Type::KeyDown:
                return "KeyDown";
            case Type::KeyUp:
                return "KeyUp";
            case Type::KeyPressEvent:
                return "KeyPressEvent";
            case Type::PaintEvent:
                return "PaintEvent";
            case Type::WindowResize:
                return "WindowResize";
            case Type::WindowClose:
                return "WindowClose";
            case Type::Unknown:
                default:
                    return "Unknown";
        }
    }
};
```

---

### Файл: `src/ui/event_listener.cppm`

```cpp
module;
#include <memory>
import ui.event;

export module ui.event_listener;

export class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void handleEvent(const Event& event) = 0;
};
```

---

### Файл: `src/ui/file_browser.cppm`

```cpp
module;
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <functional>

export module ui.file_browser;

import state.app_signals;
import ui.widget;
import ui.render_buffer;
import ui.event;

export class FileBrowser : public Widget {
private:
    struct FileEntry {
        std::string name;
        bool isDirectory;
        bool isParentDir;

        FileEntry(const std::string& name, bool isDir, bool isParent = false)
            : name(name), isDirectory(isDir), isParentDir(isParent) {}
    };

    std::filesystem::path currentPath;
    std::vector<FileEntry> entries;
    int selectedIndex = 0;
    int scrollOffset = 0;

    XftFont* font = nullptr;
    XftColor* textColor = nullptr;

    unsigned long backgroundColor = 0xFFFFFF;  // White
    unsigned long borderColor = 0x000000;      // Black

    int lineHeight = 20;
    int padding = 5;

    std::function<void(const std::filesystem::path&)> onFileSelected;

    // Scrollbar constants
    const int scrollbarWidth = 10;
    const int scrollbarMinLength = 20;

    // For double-click detection
    Time lastClickTime = 0;

    bool focused = false;
    unsigned long focusedBorderColor = 0x0000FF; // Blue

public:
    FileBrowser(int x, int y, int width, int height, const std::string& path = "", const std::string& widgetId = "")
        : Widget(widgetId), currentPath(path.empty() ? std::filesystem::current_path() : std::filesystem::path(path)) {
        // Set position and size manually
        setX(x);
        setY(y);
        setWidth(width);
        setHeight(height);

        // Initialize file browser
        refreshEntries();
    }

    ~FileBrowser() {
        if (textColor) {
            RenderBuffer* currentBuffer = getBuffer();
            if (currentBuffer) {
                currentBuffer->freeXftColor(textColor);
            }
            textColor = nullptr;
        }
    }

    bool hasFocus() const {
        return focused;
    }

    void setFocus(bool focus) {
        if (focused != focus) {
            focused = focus;
            markDirty();
        }
    }

    void setFont(XftFont* newFont) {
        font = newFont;
        if (font) {
            lineHeight = font->ascent + font->descent + 2;
        }
    }

    void setPath(const std::filesystem::path& path) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            currentPath = path;
            refreshEntries();
            selectedIndex = 0;
            scrollOffset = 0;
            markDirty();
        }
    }

    std::filesystem::path getCurrentPath() const {
        return currentPath;
    }

    void setOnFileSelected(std::function<void(const std::filesystem::path&)> callback) {
        onFileSelected = callback;
    }

    void refreshEntries() {
        entries.clear();

        // Add parent directory entry if not at root
        if (currentPath.has_parent_path() && currentPath != currentPath.parent_path()) {
            entries.emplace_back("..", true, true);
        }

        try {
            // Add directories first
            for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                if (entry.is_directory()) {
                    entries.emplace_back(entry.path().filename().string(), true);
                }
            }

            // Then add files
            for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                if (!entry.is_directory()) {
                    entries.emplace_back(entry.path().filename().string(), false);
                }
            }

            // Sort entries alphabetically within their groups
            auto dirEnd = std::partition(entries.begin(), entries.end(),
                [](const FileEntry& e) { return e.isDirectory; });

            std::sort(entries.begin(), dirEnd,
                [](const FileEntry& a, const FileEntry& b) {
                    if (a.isParentDir) return true;
                    if (b.isParentDir) return false;
                    return a.name < b.name;
                });

            std::sort(dirEnd, entries.end(),
                [](const FileEntry& a, const FileEntry& b) {
                    return a.name < b.name;
                });
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error accessing directory: " << e.what() << std::endl;
        }
    }

    void handleEvent(const Event& event) override {
        bool isInside = containsPoint(event.getX(), event.getY());

        if (event.getType() == Event::Type::MouseDown) {
            // Устанавливаем фокус, если клик внутри виджета
            bool wasFocused = focused;
            focused = isInside;

            if (focused != wasFocused) {
                markDirty();
            }

            if (isInside) {
                // Calculate which entry was clicked
                int y = event.getY() - getY() - padding;
                int clickedIndex = scrollOffset + (y / lineHeight);

                if (clickedIndex >= 0 && clickedIndex < static_cast<int>(entries.size())) {
                    selectedIndex = clickedIndex;

                    // Handle double click to navigate or select file
                    if (event.getNativeEvent().xbutton.button == Button1 &&
                        event.getNativeEvent().xbutton.time - lastClickTime < 300) {
                        handleEntryActivation();
                    }

                    lastClickTime = event.getNativeEvent().xbutton.time;
                    markDirty();
                }
            }
        }
        else if (event.getType() == Event::Type::KeyDown && focused) {
            XKeyEvent keyEvent = event.getNativeEvent().xkey;
            KeySym keysym = XLookupKeysym(&keyEvent, 0);

            if (keysym == XK_Up) {
                if (selectedIndex > 0) {
                    selectedIndex--;
                    ensureSelectedVisible();
                    markDirty();
                }
            }
            else if (keysym == XK_Down) {
                if (selectedIndex < static_cast<int>(entries.size()) - 1) {
                    selectedIndex++;
                    ensureSelectedVisible();
                    markDirty();
                }
            }
            else if (keysym == XK_Page_Up) {
                int visibleLines = (getHeight() - 2 * padding - lineHeight) / lineHeight;
                selectedIndex = std::max(0, selectedIndex - visibleLines);
                ensureSelectedVisible();
                markDirty();
            }
            else if (keysym == XK_Page_Down) {
                int visibleLines = (getHeight() - 2 * padding - lineHeight) / lineHeight;
                selectedIndex = std::min(static_cast<int>(entries.size()) - 1, selectedIndex + visibleLines);
                ensureSelectedVisible();
                markDirty();
            }
            else if (keysym == XK_Return || keysym == XK_KP_Enter) {
                handleEntryActivation();
            }
            else if (keysym == XK_BackSpace) {
                // Go up one directory
                if (currentPath.has_parent_path() && currentPath != currentPath.parent_path()) {
                    setPath(currentPath.parent_path());
                }
            }
        }

        Widget::handleEvent(event);
    }

    void handleEntryActivation() {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(entries.size())) {
            const auto& entry = entries[selectedIndex];

            const auto fullPath = currentPath / entry.name;

            if (entry.isDirectory) {
                // Navigate to directory
                if (entry.isParentDir) {
                    setPath(currentPath.parent_path());
                } else {
                    setPath(fullPath);
                }
            } else if (onFileSelected) {
                onFileSelected(fullPath);
            }

            getFileSelectedSignal()->emit({
                .filePath = fullPath,
                .fileName = entry.name,
                .isDirectory = entry.isDirectory
            });
        }
    }

    void ensureSelectedVisible() {
        int headerHeight = padding + lineHeight + padding/2 + padding; // Path + separator
        int visibleLines = (getHeight() - headerHeight - padding) / lineHeight;

        // If selected item is above the visible area
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
        // If selected item is below the visible area
        else if (selectedIndex >= scrollOffset + visibleLines) {
            scrollOffset = selectedIndex - visibleLines + 1;
        }

        // Ensure scroll offset is valid
        if (scrollOffset < 0) scrollOffset = 0;
        int maxScroll = std::max(0, static_cast<int>(entries.size()) - visibleLines);
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    }

    void paintToBuffer(Display* display) override {
        ensureBuffer(display, getWindow());
        RenderBuffer* currentBuffer = getBuffer();
        if (!currentBuffer || !font) return;

        int width = getWidth();
        int height = getHeight();

        // Clear background
        currentBuffer->clear(backgroundColor);

        // Draw border with focus color if focused
        unsigned long  borderColorToUse = focused ? focusedBorderColor : borderColor;
        currentBuffer->drawRectangle(0, 0, width - 1, height - 1, borderColorToUse);

        // Initialize text color if needed
        if (!textColor) {
            textColor = currentBuffer->createXftColor(0, 0, 0);  // Black
        }

        // Draw current path
        std::string pathStr = currentPath.string();
        int pathY = padding + font->ascent;
        currentBuffer->drawText(padding, pathY, pathStr, font, textColor);

        // Draw separator line
        int separatorY = padding + lineHeight + padding/2;
        currentBuffer->drawLine(padding, separatorY, width - padding, separatorY, borderColor);

        // Draw file entries
        int startY = separatorY + padding;
        int visibleLines = (height - startY - padding) / lineHeight;

        // Ensure scroll offset is valid
        if (scrollOffset > static_cast<int>(entries.size()) - visibleLines) {
            scrollOffset = std::max(0, static_cast<int>(entries.size()) - visibleLines);
        }

        for (int i = 0; i < visibleLines && i + scrollOffset < static_cast<int>(entries.size()); i++) {
            int entryIndex = i + scrollOffset;
            const auto& entry = entries[entryIndex];

            int entryY = startY + i * lineHeight;

            // Draw selection highlight
            if (entryIndex == selectedIndex) {
                currentBuffer->fillRectangle(padding, entryY, width - 2 * padding, lineHeight, 0xADD8E6);
            }

            // Draw entry icon/prefix
            std::string prefix = entry.isDirectory ? "[DIR] " : "     ";
            if (entry.isParentDir) prefix = "[..] ";

            // Draw entry text
            int textY = entryY + font->ascent;
            currentBuffer->drawText(padding * 2, textY, prefix + entry.name, font, textColor);
        }

        // Draw scrollbar if needed
        if (entries.size() > visibleLines) {
            int scrollbarX = width - scrollbarWidth - 1;
            int scrollbarY = startY;
            int scrollbarHeight = height - startY - padding;

            // Draw scrollbar background
            currentBuffer->fillRectangle(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, 0xDDDDDD);

            // Calculate thumb position and size
            float thumbRatio = static_cast<float>(visibleLines) / entries.size();
            int thumbHeight = std::max(static_cast<int>(scrollbarHeight * thumbRatio), scrollbarMinLength);
            int thumbY = scrollbarY + static_cast<int>((scrollbarHeight - thumbHeight) *
                                                     (static_cast<float>(scrollOffset) /
                                                      std::max(1, static_cast<int>(entries.size() - visibleLines))));

            // Draw thumb
            currentBuffer->fillRectangle(scrollbarX, thumbY, scrollbarWidth, thumbHeight, 0x888888);
        }
    }
};
```

---

### Файл: `src/ui/image.cppm`

```cpp
module;
#include <string>
#include <memory>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Imlib2.h>  // Для работы с изображениями
#include <fstream>
#include <algorithm>

export module ui.image;

import ui.widget;
import ui.render_buffer;
import services.logger_service;
import core.ioc_container;

export class Image : public Widget {
private:
    std::string imagePath;
    Pixmap pixmap = 0;
    bool imageLoaded = false;
    int imageWidth = 0;
    int imageHeight = 0;
    Display* currentDisplay = nullptr;
    std::shared_ptr<LoggerService> logger;

public:
    Image(const std::string& widgetId, int x, int y, int w, int h, const std::string& path)
        : Widget(widgetId) {
        setPosition(x, y);
        setSize(w, h);
        imagePath = path;
        logger = std::make_shared<LoggerService>();
        logger->info("Creating Image widget with path: " + path);

        // Проверяем существование файла
        std::ifstream file(path);
        if (!file.good()) {
            logger->error("Image file does not exist or cannot be accessed: " + path);
        } else {
            logger->info("Image file exists and is accessible");
        }
    }

    ~Image() {
        if (pixmap && currentDisplay) {
            XFreePixmap(currentDisplay, pixmap);
        }
    }

    void loadImage(Display* display) {
        logger->info("loadImage called for: " + imagePath);

        if (!display) {
            logger->error("Display is null, cannot load image");
            return;
        }

        currentDisplay = display;
        logger->info("Display set, attempting to load image with Imlib2");

        // Сохраняем текущий контекст Imlib
        Imlib_Context prev_context = imlib_context_get();
        logger->info("Saved previous Imlib context");

        // Используем Imlib2 для загрузки изображения
        try {
            // Инициализируем Imlib для работы с этим дисплеем
            imlib_context_set_display(display);
            imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
            imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));
            logger->info("Imlib context initialized for display");

            logger->info("Calling imlib_load_image");
            Imlib_Image img = imlib_load_image(imagePath.c_str());
            if (!img) {
                logger->error("Failed to load image: " + imagePath);
                return;
            }
            logger->info("Image loaded successfully with Imlib2");

            logger->info("Setting image context");
            imlib_context_set_image(img);

            // Получаем размеры изображения
            imageWidth = imlib_image_get_width();
            imageHeight = imlib_image_get_height();
            logger->info("Image dimensions: " + std::to_string(imageWidth) + "x" + std::to_string(imageHeight));

            // Создаем Pixmap для X11
            logger->info("Creating X11 Pixmap");
            pixmap = XCreatePixmap(display, DefaultRootWindow(display),
                                imageWidth, imageHeight, DefaultDepth(display, DefaultScreen(display)));
            if (!pixmap) {
                logger->error("Failed to create pixmap");
                imlib_free_image();
                return;
            }
            logger->info("Pixmap created successfully");

            // Рендерим изображение в Pixmap
            logger->info("Setting drawable and rendering image");
            imlib_context_set_drawable(pixmap);
            imlib_render_image_on_drawable(0, 0);
            logger->info("Image rendered to drawable");

            // Освобождаем ресурсы Imlib
            logger->info("Freeing Imlib image resources");
            imlib_free_image();

            imageLoaded = true;
            markDirty();
            logger->info("Image loaded and marked dirty");
        } catch (const std::exception& e) {
            logger->error("Exception during image loading: " + std::string(e.what()));
        } catch (...) {
            logger->error("Unknown exception during image loading");
        }
    }

    void paintToBuffer(Display* display) override {
        logger->info("paintToBuffer called");

        if (!display) {
            logger->error("Display is null, cannot paint to buffer");
            return;
        }

        // Сохраняем указатель на дисплей для использования в деструкторе
        currentDisplay = display;

        // Если изображение еще не загружено, загружаем его
        if (!imageLoaded) {
            logger->info("Image not loaded, calling loadImage");
            loadImage(display);
        }

        if (!imageLoaded || !pixmap) {
            logger->error("Image not loaded or pixmap not created after loading attempt");
            return;
        }

        // Получаем буфер для рисования
        RenderBuffer* buffer = getBuffer();
        if (!buffer) {
            logger->error("No render buffer available");
            return;
        }
        logger->info("Render buffer obtained");

        try {
            // Используем Imlib2 для масштабирования изображения
            logger->info("Using Imlib2 for image scaling");

            // Use RAII pattern for Imlib context management
            Imlib_Context prev_context = imlib_context_get();
//            imlib_context_push();
            auto contextGuard = [prev_context]() {
                imlib_context_free(prev_context);
//                imlib_context_pop();
            };

            // Load the image
            Imlib_Image img = imlib_load_image(imagePath.c_str());
            if (!img) {
                logger->error("Failed to load image for scaling");
                contextGuard();  // Restore context before returning
                return;
            }

            imlib_context_set_image(img);

            // Вычисляем параметры для режима "cover"
            double widgetRatio = (double)getWidth() / getHeight();
            double imageRatio = (double)imageWidth / imageHeight;

            int targetWidth = getWidth();
            int targetHeight = getHeight();

            // Масштабируем изображение с сохранением пропорций
            Imlib_Image scaled_img;
            if (imageRatio > widgetRatio) {
                // Изображение шире, чем виджет (относительно)
                int newWidth = (int)(imageHeight * widgetRatio);
                int offsetX = (imageWidth - newWidth) / 2;

                // Обрезаем и масштабируем
                scaled_img = imlib_create_cropped_scaled_image(
                    offsetX, 0, newWidth, imageHeight,
                    targetWidth, targetHeight
                );
            } else {
                // Изображение выше, чем виджет (относительно)
                int newHeight = (int)(imageWidth / widgetRatio);
                int offsetY = (imageHeight - newHeight) / 2;

                // Обрезаем и масштабируем
                scaled_img = imlib_create_cropped_scaled_image(
                    0, offsetY, imageWidth, newHeight,
                    targetWidth, targetHeight
                );
            }

            // Освобождаем исходное изображение
            imlib_free_image();

            if (!scaled_img) {
                logger->error("Failed to scale image");
                return;
            }

            // Устанавливаем масштабированное изображение как текущее
            imlib_context_set_image(scaled_img);

            // Настраиваем контекст для рендеринга
            imlib_context_set_display(display);
            imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
            imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));
            imlib_context_set_drawable(buffer->getPixmap());

            // Рендерим масштабированное изображение прямо в буфер виджета
            imlib_render_image_on_drawable(0, 0);

            // Освобождаем масштабированное изображение
            imlib_free_image();

            // Восстанавливаем предыдущий контекст Imlib
            imlib_context_free(prev_context);
//            imlib_context_pop();

            logger->info("Image scaled and rendered to buffer successfully");
        } catch (const std::exception& e) {
            logger->error("Exception during painting: " + std::string(e.what()));
        } catch (...) {
            logger->error("Unknown exception during painting");
        }
    }

    void setImage(const std::string& newImagePath) {
        logger->info("Changing image to: " + newImagePath);

        // Check if the new file exists
        std::ifstream file(newImagePath);
        if (!file.good()) {
            logger->error("New image file does not exist or cannot be accessed: " + newImagePath);
            return;
        }

        // Clean up old resources if needed
        if (pixmap && currentDisplay) {
            XFreePixmap(currentDisplay, pixmap);
            pixmap = 0;
        }

        // Set the new path
        imagePath = newImagePath;
        imageLoaded = false;

        // Force reload and repaint
        markDirty();

        logger->info("Image changed successfully, will be loaded on next paint");
    }
};
```

---

### Файл: `src/ui/render_buffer.cppm`

```cpp
module;
#include <X11/Xlib.h>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <string>
#include <X11/Xft/Xft.h>

export module ui.render_buffer;

export class RenderBuffer {
private:
    Display* display;
    Pixmap pixmap;
    GC gc;
    unsigned int width;
    unsigned int height;
    bool valid = true;  // Флаг для проверки валидности буфера
    XftDraw* xftDraw = nullptr;

public:
    RenderBuffer(Display* display, ::Window window, unsigned int width, unsigned int height)
        : display(display), width(width), height(height) {

        if (!display) {
            throw std::invalid_argument("Display pointer cannot be null");
        }

        if (width == 0 || height == 0) {
            throw std::invalid_argument("Width and height must be greater than zero");
        }

        // Create an off-screen pixmap
        pixmap = XCreatePixmap(display, window, width, height,
                              DefaultDepth(display, DefaultScreen(display)));

        if (!pixmap) {
            throw std::runtime_error("Failed to create pixmap");
        }

        // Create a GC for drawing on this pixmap
        gc = XCreateGC(display, pixmap, 0, nullptr);

        if (!gc) {
            XFreePixmap(display, pixmap);
            throw std::runtime_error("Failed to create graphics context");
        }

        // Create XftDraw for text rendering
        Visual* visual = DefaultVisual(display, DefaultScreen(display));
        Colormap colormap = DefaultColormap(display, DefaultScreen(display));
        xftDraw = XftDrawCreate(display, pixmap, visual, colormap);

        if (!xftDraw) {
            XFreeGC(display, gc);
            XFreePixmap(display, pixmap);
            throw std::runtime_error("Failed to create Xft draw context");
        }

        // Clear the pixmap initially
        XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
        XFillRectangle(display, pixmap, gc, 0, 0, width, height);
    }

    ~RenderBuffer() {
        if (valid) {
            if (xftDraw) {
                XftDrawDestroy(xftDraw);
            }
            XFreeGC(display, gc);
            XFreePixmap(display, pixmap);
        }
    }

    // No copying
    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;

    // Moving is allowed
    RenderBuffer(RenderBuffer&& other) noexcept
        : display(other.display), pixmap(other.pixmap), gc(other.gc),
          width(other.width), height(other.height), valid(other.valid),
          xftDraw(other.xftDraw) {
        other.pixmap = 0;
        other.gc = nullptr;
        other.xftDraw = nullptr;
        other.valid = false;
    }

    RenderBuffer& operator=(RenderBuffer&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (valid) {
                if (xftDraw) {
                    XftDrawDestroy(xftDraw);
                }
                XFreeGC(display, gc);
                XFreePixmap(display, pixmap);
            }

            // Move resources from other
            display = other.display;
            pixmap = other.pixmap;
            gc = other.gc;
            width = other.width;
            height = other.height;
            valid = other.valid;
            xftDraw = other.xftDraw;

            // Invalidate other
            other.pixmap = 0;
            other.gc = nullptr;
            other.xftDraw = nullptr;
            other.valid = false;
        }
        return *this;
    }

    // Get the pixmap for blitting
    Pixmap getPixmap() const { return pixmap; }

    // Get the GC for drawing
    GC getGC() const { return gc; }

    // Clear the buffer
    void clear(unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XFillRectangle(display, pixmap, gc, 0, 0, width, height);
    }

    // Добавленные методы для рисования

    // Рисование линии
    void drawLine(int x1, int y1, int x2, int y2, unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XDrawLine(display, pixmap, gc, x1, y1, x2, y2);
    }

    // Рисование прямоугольника (контур)
    void drawRectangle(int x, int y, unsigned int width, unsigned int height, unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XDrawRectangle(display, pixmap, gc, x, y, width, height);
    }

    // Рисование закрашенного прямоугольника
    void fillRectangle(int x, int y, unsigned int width, unsigned int height, unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XFillRectangle(display, pixmap, gc, x, y, width, height);
    }

    // Рисование текста с использованием Xft
    void drawText(int x, int y, const std::string& text, XftFont* font, XftColor* color) {
        if (!valid || !xftDraw || !font || !color) return;

        XftDrawString8(xftDraw, color, font, x, y,
                      (const FcChar8*)text.c_str(), text.length());
    }

    // Создание цвета Xft из RGB
    XftColor* createXftColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        XftColor* color = new XftColor();
        XRenderColor renderColor;
        renderColor.red = r * 257;    // Convert 0-255 to 0-65535
        renderColor.green = g * 257;
        renderColor.blue = b * 257;
        renderColor.alpha = a * 257;

        Colormap colormap = DefaultColormap(display, DefaultScreen(display));
        XftColorAllocValue(display, DefaultVisual(display, DefaultScreen(display)),
                          colormap, &renderColor, color);

        return color;
    }

    // Освобождение цвета Xft
    void freeXftColor(XftColor* color) {
        if (color) {
            Colormap colormap = DefaultColormap(display, DefaultScreen(display));
            XftColorFree(display, DefaultVisual(display, DefaultScreen(display)),
                        colormap, color);
            delete color;
        }
    }

    // Получение размеров текста
    void getTextExtents(XftFont* font, const std::string& text,
                        int& width, int& height, int& ascent, int& descent) {
        if (!font) {
            width = height = ascent = descent = 0;
            return;
        }

        XGlyphInfo extents;
        XftTextExtents8(display, font, (const FcChar8*)text.c_str(), text.length(), &extents);

        width = extents.width;
        height = extents.height;
        ascent = font->ascent;
        descent = font->descent;
    }

    void copyTo(Drawable dest, int srcX, int srcY, int destX, int destY,
                unsigned int width, unsigned int height) {
        if (!valid) {
            std::cout << "DEBUG: RenderBuffer::copyTo - Buffer is not valid!" << std::endl;
            return;
        }

//        std::cout << "DEBUG: RenderBuffer::copyTo - Copying from (" << srcX << "," << srcY
//                  << ") to (" << destX << "," << destY << ") with size "
//                  << width << "x" << height << std::endl;

        // Проверка границ
        if (srcX < 0) srcX = 0;
        if (srcY < 0) srcY = 0;
        if (srcX + width > this->width) width = this->width - srcX;
        if (srcY + height > this->height) height = this->height - srcY;

        if (width > 0 && height > 0) {
//            std::cout << "DEBUG: RenderBuffer::copyTo - After bounds check: from (" << srcX << "," << srcY
//                      << ") with size " << width << "x" << height << std::endl;

            // Создаем временный GC для операции копирования
            GC tempGC = XCreateGC(display, dest, 0, nullptr);
            if (tempGC) {
//                std::cout << "DEBUG: RenderBuffer::copyTo - Using temporary GC" << std::endl;
                XCopyArea(display, pixmap, dest, tempGC, srcX, srcY, width, height, destX, destY);
                XFreeGC(display, tempGC);
            } else {
//                std::cout << "DEBUG: RenderBuffer::copyTo - Using buffer's GC" << std::endl;
                XCopyArea(display, pixmap, dest, gc, srcX, srcY, width, height, destX, destY);
            }
            XFlush(display);
        } else {
            std::cout << "DEBUG: RenderBuffer::copyTo - Nothing to copy after bounds check" << std::endl;
        }
    }

    // Упрощенная версия copyTo для копирования всего буфера
    void copyToWindow(::Window window, int x, int y) {
        copyTo(window, 0, 0, x, y, width, height);
    }

    // Проверка валидности буфера
    bool isValid() const { return valid; }

    // Изменение размера буфера
    bool resize(::Window window, unsigned int newWidth, unsigned int newHeight) {
        if (!valid || newWidth == 0 || newHeight == 0) return false;

        // Создаем новый pixmap
        Pixmap newPixmap = XCreatePixmap(display, window, newWidth, newHeight,
                                        DefaultDepth(display, DefaultScreen(display)));
        if (!newPixmap) return false;

        // Копируем содержимое старого pixmap в новый
        unsigned int copyWidth = std::min(width, newWidth);
        unsigned int copyHeight = std::min(height, newHeight);
        XCopyArea(display, pixmap, newPixmap, gc, 0, 0, copyWidth, copyHeight, 0, 0);

        // Освобождаем старый pixmap
        XFreePixmap(display, pixmap);

        // Обновляем данные
        pixmap = newPixmap;
        width = newWidth;
        height = newHeight;

        // Очищаем новые области, если буфер увеличился
        if (newWidth > copyWidth) {
            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
            XFillRectangle(display, pixmap, gc, copyWidth, 0, newWidth - copyWidth, copyHeight);
        }

        if (newHeight > copyHeight) {
            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
            XFillRectangle(display, pixmap, gc, 0, copyHeight, newWidth, newHeight - copyHeight);
        }

        return true;
    }
    
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

    // Draw an XImage to the buffer
    void drawImage(XImage* image, int x, int y) {
        if (!image || !image->data) return;

        // Create a GC for the pixmap
        GC gc = XCreateGC(display, pixmap, 0, nullptr);

        // Put the image on the pixmap
        XPutImage(display, pixmap, gc, image,
                 0, 0,    // Source x, y
                 x, y,    // Destination x, y
                 image->width, image->height);

        // Free the GC
        XFreeGC(display, gc);
    }
};
```

---

### Файл: `src/ui/text_area.cppm`

```cpp
module;
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

export module ui.text_area;

import ui.widget;
import ui.render_buffer;
import ui.event;

export class TextArea : public Widget {
private:
    std::vector<std::string> lines;
    std::string placeholder;
    XftFont* font = nullptr;
    XftColor* textColor = nullptr;
    XftColor* placeholderColor = nullptr;

    unsigned long backgroundColor = 0xFFFFFF;  // White
    unsigned long borderColor = 0x000000;      // Black
    unsigned long focusedBorderColor = 0x0000FF; // Blue

    int cursorRow = 0;
    int cursorCol = 0;
    bool focused = false;
    int padding = 5;  // Padding inside the text area

    int scrollX = 0;  // Horizontal scroll position
    int scrollY = 0;  // Vertical scroll position
    int lineHeight = 0; // Will be calculated based on font

    bool hasSelection = false;
    int selectionStartRow = 0;
    int selectionStartCol = 0;
    int selectionEndRow = 0;
    int selectionEndCol = 0;
    unsigned long selectionColor = 0xADD8E6; // Light blue

    std::string clipboardText;

    // Constants for scrollbar
    static constexpr int scrollbarWidth = 10;
    static constexpr int scrollbarMinLength = 20;
    unsigned long scrollbarColor = 0x888888;
    unsigned long scrollbarThumbColor = 0x444444;

public:
    TextArea(int x, int y, int width, int height, const std::string& placeholder = "")
        : Widget(placeholder) {  // Use placeholder as the widget ID or use an empty string
        // Set position and size manually
        setX(x);
        setY(y);
        setWidth(width);
        setHeight(height);
        this->placeholder = placeholder;

        // Initialize with one empty line
        lines.push_back("");
    }

    ~TextArea() {
        RenderBuffer* currentBuffer = getBuffer();
        if (currentBuffer) {
            if (textColor) {
                currentBuffer->freeXftColor(textColor);
                textColor = nullptr;
            }
            if (placeholderColor) {
                currentBuffer->freeXftColor(placeholderColor);
                placeholderColor = nullptr;
            }
        }
        // Font should be loaded and freed by the application
    }

    void setText(const std::string& newText) {
        // Split the text into lines
        lines.clear();
        size_t start = 0;
        size_t end = newText.find('\n');

        while (end != std::string::npos) {
            lines.push_back(newText.substr(start, end - start));
            start = end + 1;
            end = newText.find('\n', start);
        }

        // Add the last line
        lines.push_back(newText.substr(start));

        // If no text was provided, ensure we have at least one empty line
        if (lines.empty()) {
            lines.push_back("");
        }

        // Set cursor at the end of the text
        cursorRow = lines.size() - 1;
        cursorCol = lines[cursorRow].length();

        markDirty();
    }

    std::string getText() const {
        std::string result;
        for (size_t i = 0; i < lines.size(); ++i) {
            result += lines[i];
            if (i < lines.size() - 1) {
                result += '\n';
            }
        }
        return result;
    }

    void setFont(XftFont* newFont) {
        font = newFont;
        if (font) {
            // Calculate line height based on font metrics
            lineHeight = font->ascent + font->descent + 2; // +2 for a little extra spacing
        }
    }

    void setFocus(bool focus) {
        focused = focus;
    }

    bool isFocused() const {
        return focused;
    }

    void setBackgroundColor(unsigned long color) {
        backgroundColor = color;
    }

    void setBorderColor(unsigned long color) {
        borderColor = color;
    }

    void setFocusedBorderColor(unsigned long color) {
        focusedBorderColor = color;
    }

    void handleEvent(const Event& event) override {
        // Check if the point is inside the text area
        bool isInside = containsPoint(event.getX(), event.getY());

        if (event.getType() == Event::Type::MouseDown && isInside) {
            std::cerr << "TextArea clicked, setting focus to true" << std::endl;
            setFocus(true);

            // Set cursor position based on click position
            setCursorPositionFromMouse(event.getX(), event.getY());

            // Start selection
            startSelection();

            markDirty();
        }
        else if (event.getType() == Event::Type::MouseMove && isInside) {
            // Update selection while dragging
            if (event.getNativeEvent().xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
                setCursorPositionFromMouse(event.getX(), event.getY());
                updateSelection();
                markDirty();
            }
        }
        else if (event.getType() == Event::Type::MouseUp) {
            // Finalize selection
            if (selectionStartRow == cursorRow && selectionStartCol == cursorCol) {
                clearSelection();
            }
            markDirty();
        }
        else if (event.getType() == Event::Type::MouseDown && !isInside) {
            std::cerr << "Click outside TextArea, removing focus" << std::endl;
            setFocus(false);
            clearSelection();
            markDirty(); // Mark as dirty when losing focus
        }

        // Handle keyboard events if focused
        if (focused && event.getType() == Event::Type::KeyDown) {
            std::cerr << "TextArea processing key press event" << std::endl;
            XKeyEvent keyCopy = event.getNativeEvent().xkey;
            handleKeyPress(keyCopy);
            markDirty();
        }

        // Pass the event to the base class
        Widget::handleEvent(event);
    }

    void setCursorPositionFromMouse(int mouseX, int mouseY) {
        if (!font) return;

        // Get the buffer from the parent class instead of using the member variable directly
        RenderBuffer* currentBuffer = getBuffer();
        if (!currentBuffer) return;

        // Adjust for padding and scroll
        int localX = mouseX - getX() - padding + scrollX;
        int localY = mouseY - getY() - padding + scrollY * lineHeight;

        // Find the row
        cursorRow = localY / lineHeight;
        if (cursorRow < 0) cursorRow = 0;
        if (cursorRow >= static_cast<int>(lines.size())) {
            cursorRow = lines.size() - 1;
        }

        // Find the column by measuring text widths
        const std::string& line = lines[cursorRow];
        cursorCol = 0;

        if (localX <= 0) {
            cursorCol = 0;
        } else {
            // Binary search to find the closest character position
            int left = 0;
            int right = line.length();

            while (left < right) {
                int mid = (left + right) / 2;
                std::string textToMid = line.substr(0, mid);

                int textWidth, textHeight, textAscent, textDescent;
                currentBuffer->getTextExtents(font, textToMid, textWidth, textHeight, textAscent, textDescent);

                if (textWidth < localX) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }

            cursorCol = left;

            // Check if we should place cursor before or after the character
            if (cursorCol > 0) {
                std::string textBefore = line.substr(0, cursorCol - 1);
                std::string textAt = line.substr(0, cursorCol);

                int widthBefore, heightBefore, ascentBefore, descentBefore;
                int widthAt, heightAt, ascentAt, descentAt;

                currentBuffer->getTextExtents(font, textBefore, widthBefore, heightBefore, ascentBefore, descentBefore);
                currentBuffer->getTextExtents(font, textAt, widthAt, heightAt, ascentAt, descentAt);

                if (localX - widthBefore < widthAt - localX) {
                    cursorCol--;
                }
            }
        }

        ensureCursorVisible();
    }

    bool handleKeyPress(XKeyEvent& event) {
        if (!focused) return false;

        char buffer[32];
        KeySym keysym;
        int count = XLookupString(&event, buffer, sizeof(buffer), &keysym, nullptr);

        bool result = false;

        // Check for modifier keys
        bool ctrlPressed = (event.state & ControlMask);
        bool shiftPressed = (event.state & ShiftMask);

        // Handle copy/paste shortcuts
        if (ctrlPressed && !shiftPressed) {
            if (keysym == XK_c || keysym == XK_C) {
                // Ctrl+C: Copy
                copyToClipboard();
                return true;
            } else if (keysym == XK_v || keysym == XK_V) {
                // Ctrl+V: Paste
                pasteFromClipboard();
                return true;
            } else if (keysym == XK_x || keysym == XK_X) {
                // Ctrl+X: Cut
                copyToClipboard();
                deleteSelectedText();
                return true;
            } else if (keysym == XK_a || keysym == XK_A) {
                // Ctrl+A: Select All
                selectionStartRow = 0;
                selectionStartCol = 0;
                selectionEndRow = lines.size() - 1;
                selectionEndCol = lines[selectionEndRow].length();
                hasSelection = true;
                return true;
            }
        }

        // Start selection if Shift is pressed and we don't have a selection yet
        if (shiftPressed && !hasSelection) {
            startSelection();
        }

        // Handle navigation keys with selection
        if (keysym == XK_BackSpace) {
            if (hasSelection) {
                deleteSelectedText();
                result = true;
            } else {
                result = handleBackspace();
            }
        } else if (keysym == XK_Delete) {
            if (hasSelection) {
                deleteSelectedText();
                result = true;
            } else {
                result = handleDelete();
            }
        } else if (keysym == XK_Left) {
            if (ctrlPressed) {
                // Ctrl+Left: Move to previous word
                if (cursorCol > 0) {
                    auto boundaries = findWordBoundaries(cursorRow, cursorCol - 1);
                    cursorCol = boundaries.first;
                } else if (cursorRow > 0) {
                    cursorRow--;
                    cursorCol = lines[cursorRow].length();
                }
            } else {
                // Regular left arrow
                if (cursorCol > 0) {
                    cursorCol--;
                } else if (cursorRow > 0) {
                    cursorRow--;
                    cursorCol = lines[cursorRow].length();
                }
            }

            if (shiftPressed) {
                // Update selection if Shift is pressed
                updateSelection();
            } else {
                // Clear selection if no Shift
                clearSelection();
            }
            result = true;
        } else if (keysym == XK_Right) {
            if (ctrlPressed) {
                // Ctrl+Right: Move to next word
                if (cursorCol < static_cast<int>(lines[cursorRow].length())) {
                    auto boundaries = findWordBoundaries(cursorRow, cursorCol);
                    cursorCol = boundaries.second;
                } else if (cursorRow < static_cast<int>(lines.size()) - 1) {
                    cursorRow++;
                    cursorCol = 0;
                }
            } else {
                // Regular right arrow
                if (cursorCol < static_cast<int>(lines[cursorRow].length())) {
                    cursorCol++;
                } else if (cursorRow < static_cast<int>(lines.size()) - 1) {
                    cursorRow++;
                    cursorCol = 0;
                }
            }

            if (shiftPressed) {
                // Update selection if Shift is pressed
                updateSelection();
            } else {
                // Clear selection if no Shift
                clearSelection();
            }
            result = true;
        } else if (keysym == XK_Up) {
            if (cursorRow > 0) {
                cursorRow--;
                // Adjust column if the new line is shorter
                cursorCol = std::min(cursorCol, static_cast<int>(lines[cursorRow].length()));
            }

            if (shiftPressed) {
                updateSelection();
            } else {
                clearSelection();
            }
            result = true;
        } else if (keysym == XK_Down) {
            if (cursorRow < static_cast<int>(lines.size()) - 1) {
                cursorRow++;
                // Adjust column if the new line is shorter
                cursorCol = std::min(cursorCol, static_cast<int>(lines[cursorRow].length()));
            }

            if (shiftPressed) {
                updateSelection();
            } else {
                clearSelection();
            }
            result = true;
        } else if (keysym == XK_Home) {
            cursorCol = 0;

            if (shiftPressed) {
                updateSelection();
            } else {
                clearSelection();
            }
            result = true;
        } else if (keysym == XK_End) {
            cursorCol = lines[cursorRow].length();

            if (shiftPressed) {
                updateSelection();
            } else {
                clearSelection();
            }
            result = true;
        } else if (keysym == XK_Return || keysym == XK_KP_Enter) {
            if (hasSelection) {
                deleteSelectedText();
            }
            result = handleEnter();
        } else if (keysym == XK_Tab) {
            if (hasSelection) {
                deleteSelectedText();
            }
            result = handleTab();
        } else if (count > 0 && !ctrlPressed) {
            // Insert the typed character at cursor position
            if (hasSelection) {
                deleteSelectedText();
            }
            lines[cursorRow].insert(cursorCol, buffer, count);
            cursorCol += count;
            clearSelection();
            result = true;
        }

        if (result) {
            ensureCursorVisible();
            markDirty();
        }

        return result;
    }

void paintToBuffer(Display* display) override {
    // Get the buffer from the parent class
    ensureBuffer(display, getWindow());
    RenderBuffer* currentBuffer = getBuffer();
    if (!currentBuffer) return;

    int width = getWidth();
    int height = getHeight();

    // Clear buffer with background color
    currentBuffer->clear(backgroundColor);

    // Draw border
    unsigned long currentBorderColor = focused ? focusedBorderColor : borderColor;
    currentBuffer->drawRectangle(0, 0, width - 1, height - 1, currentBorderColor);
    std::cerr << "text area paintToBuffer drawRectangle" << std::endl;

    // Initialize font if needed
    if (!font) {
        std::cerr << "Warning: No font set for TextArea" << std::endl;
        return;
    }

    // Initialize colors if needed
    if (!textColor) {
        textColor = currentBuffer->createXftColor(0, 0, 0);  // Black
    }
    if (!placeholderColor) {
        placeholderColor = currentBuffer->createXftColor(128, 128, 128);  // Gray
    }

    // Rest of the method remains the same, but replace all instances of 'buffer' with 'currentBuffer'
    // For example:
    // buffer->drawText(...) becomes currentBuffer->drawText(...)

    // Draw text or placeholder
    bool isEmpty = lines.size() == 1 && lines[0].empty();
    XftColor* currentColor = isEmpty ? placeholderColor : textColor;

    if (isEmpty && !placeholder.empty()) {
        // Draw placeholder text
        int textWidth, textHeight, textAscent, textDescent;
        currentBuffer->getTextExtents(font, placeholder, textWidth, textHeight, textAscent, textDescent);

        // Draw text with vertical centering for placeholder
        int textY = (height + textAscent - textDescent) / 2;
        currentBuffer->drawText(padding, textY, placeholder, font, currentColor);

        // Draw cursor at beginning if focused
        if (focused) {
            currentBuffer->drawLine(padding, padding, padding, 20, currentBorderColor);
        }
    } else {
        // Draw actual text lines
        int y = padding + font->ascent;

        for (size_t i = scrollY; i < lines.size() && y < height - padding; ++i) {
            const std::string& line = lines[i];

            // Skip if line is empty and not the current cursor line
            if (line.empty() && static_cast<int>(i) != cursorRow) {
                y += lineHeight;
                continue;
            }

            // Draw selection background if needed
            if (hasSelection) {
                int startRow = std::min(selectionStartRow, selectionEndRow);
                int endRow = std::max(selectionStartRow, selectionEndRow);

                if (i >= startRow && i <= endRow) {
                    int startCol = 0;
                    int endCol = line.length();

                    if (i == startRow) {
                        startCol = std::min(selectionStartCol, selectionEndCol);
                    }
                    if (i == endRow) {
                        endCol = std::max(selectionStartCol, selectionEndCol);
                    }

                    if (startCol < endCol) {
                        std::string textBeforeSelection = line.substr(0, startCol);
                        std::string selectedText = line.substr(startCol, endCol - startCol);

                        int startX, textHeight, textAscent, textDescent;
                        int endX;

                        currentBuffer->getTextExtents(font, textBeforeSelection, startX, textHeight, textAscent, textDescent);
                        currentBuffer->getTextExtents(font, selectedText, endX, textHeight, textAscent, textDescent);

                        startX += padding - scrollX;

                        // Draw selection background
                        currentBuffer->fillRectangle(startX, y - font->ascent, endX, font->ascent + font->descent, selectionColor);
                    }
                }
            }

            // Draw the line
            currentBuffer->drawText(padding - scrollX, y, line, font, textColor);

            // Draw cursor if this is the current line and we're focused
            if (focused && static_cast<int>(i) == cursorRow) {
                std::string textBeforeCursor = line.substr(0, cursorCol);
                int cursorX, textHeight, textAscent, textDescent;
                currentBuffer->getTextExtents(font, textBeforeCursor, cursorX, textHeight, textAscent, textDescent);
                cursorX += padding - scrollX;  // Add padding and adjust for scroll

                currentBuffer->drawLine(cursorX, y - font->ascent, cursorX, y + font->descent, currentBorderColor);
            }

            y += lineHeight;
        }

        // Draw vertical scrollbar if needed
        drawVerticalScrollbar(currentBuffer, width, height);

        // Draw horizontal scrollbar if needed
        drawHorizontalScrollbar(currentBuffer, width, height);
    }
}

private:
    // Helper methods for key handling
    bool handleBackspace() {
        if (cursorCol > 0) {
            // Delete character before cursor
            lines[cursorRow].erase(cursorCol - 1, 1);
            cursorCol--;
            return true;
        } else if (cursorRow > 0) {
            // Join with previous line
            cursorCol = lines[cursorRow - 1].length();
            lines[cursorRow - 1] += lines[cursorRow];
            lines.erase(lines.begin() + cursorRow);
            cursorRow--;
            return true;
        }
        return false;
    }

    bool handleDelete() {
        if (cursorCol < static_cast<int>(lines[cursorRow].length())) {
            // Delete character at cursor
            lines[cursorRow].erase(cursorCol, 1);
            return true;
        } else if (cursorRow < static_cast<int>(lines.size()) - 1) {
            // Join with next line
            lines[cursorRow] += lines[cursorRow + 1];
            lines.erase(lines.begin() + cursorRow + 1);
            return true;
        }
        return false;
    }

    bool handleLeftArrow() {
        if (cursorCol > 0) {
            cursorCol--;
            return true;
        } else if (cursorRow > 0) {
            cursorRow--;
            cursorCol = lines[cursorRow].length();
            return true;
        }
        return false;
    }

    bool handleRightArrow() {
        if (cursorCol < static_cast<int>(lines[cursorRow].length())) {
            cursorCol++;
            return true;
        } else if (cursorRow < static_cast<int>(lines.size()) - 1) {
            cursorRow++;
            cursorCol = 0;
            return true;
        }
        return false;
    }

    bool handleUpArrow() {
        if (cursorRow > 0) {
            cursorRow--;
            // Adjust column if the new line is shorter
            cursorCol = std::min(cursorCol, static_cast<int>(lines[cursorRow].length()));
            return true;
        }
        return false;
    }

    bool handleDownArrow() {
        if (cursorRow < static_cast<int>(lines.size()) - 1) {
            cursorRow++;
            // Adjust column if the new line is shorter
            cursorCol = std::min(cursorCol, static_cast<int>(lines[cursorRow].length()));
            return true;
        }
        return false;
    }

    bool handleEnter() {
        // Split the current line at cursor position
        std::string currentLine = lines[cursorRow];
        std::string newLine = currentLine.substr(cursorCol);
        lines[cursorRow] = currentLine.substr(0, cursorCol);

        // Insert the new line after the current one
        lines.insert(lines.begin() + cursorRow + 1, newLine);

        // Move cursor to the beginning of the new line
        cursorRow++;
        cursorCol = 0;
        return true;
    }

    bool handleTab() {
        // Insert 2 spaces for a tab
        const std::string tabSpaces = "  ";
        lines[cursorRow].insert(cursorCol, tabSpaces);
        cursorCol += tabSpaces.length();
        return true;
    }

    void ensureCursorVisible() {
        if (!font) return;

        // Calculate visible area
        int visibleWidth = getWidth() - 2 * padding;
        int visibleHeight = getHeight() - 2 * padding;
        int visibleLines = visibleHeight / lineHeight;

        // Vertical scrolling
        if (cursorRow < scrollY) {
            scrollY = cursorRow;
        } else if (cursorRow >= scrollY + visibleLines) {
            scrollY = cursorRow - visibleLines + 1;
        }

        // Horizontal scrolling - need to calculate text width
        if (cursorCol > 0) {
            std::string textBeforeCursor = lines[cursorRow].substr(0, cursorCol);
            int textWidth, textHeight, textAscent, textDescent;

            RenderBuffer* currentBuffer = getBuffer();
            if (currentBuffer) {
                currentBuffer->getTextExtents(font, textBeforeCursor, textWidth, textHeight, textAscent, textDescent);

                if (textWidth < scrollX) {
                    scrollX = std::max(0, textWidth - visibleWidth / 4);
                } else if (textWidth > scrollX + visibleWidth) {
                    scrollX = textWidth - visibleWidth + 10; // 10px buffer
                }
            }
        } else {
            // If cursor is at the beginning, scroll to the start
            if (cursorCol == 0) {
                scrollX = 0;
            }
        }
    }

    // Add methods for selection
    void startSelection() {
        hasSelection = true;
        selectionStartRow = cursorRow;
        selectionStartCol = cursorCol;
        selectionEndRow = cursorRow;
        selectionEndCol = cursorCol;
    }

    void updateSelection() {
        selectionEndRow = cursorRow;
        selectionEndCol = cursorCol;
    }

    void clearSelection() {
        hasSelection = false;
    }

    bool isPositionSelected(int row, int col) {
        if (!hasSelection) return false;

        // Normalize selection coordinates
        int startRow = std::min(selectionStartRow, selectionEndRow);
        int endRow = std::max(selectionStartRow, selectionEndRow);
        int startCol = (startRow == selectionStartRow) ? selectionStartCol : selectionEndCol;
        int endCol = (endRow == selectionEndRow) ? selectionEndCol : selectionStartCol;

        if (startRow == endRow) {
            // Selection on single line
            startCol = std::min(selectionStartCol, selectionEndCol);
            endCol = std::max(selectionStartCol, selectionEndCol);
            return (row == startRow && col >= startCol && col < endCol);
        }

        // Selection spans multiple lines
        if (row < startRow || row > endRow) return false;
        if (row == startRow) return col >= startCol;
        if (row == endRow) return col < endCol;
        return true;
    }
    // Copy selected text to clipboard
    void copyToClipboard() {
        if (!hasSelection) return;

        // Get selected text
        std::string selectedText = getSelectedText();
        if (selectedText.empty()) return;

        // Store the text for later paste requests
        clipboardText = selectedText;

        // For a more complete implementation, you would need to integrate with the X11 clipboard
        // This simplified version just stores text internally
    }

    // Get text from clipboard
    void pasteFromClipboard() {
        // For a more complete implementation, you would need to integrate with the X11 clipboard
        // This simplified version just uses the internally stored text

        if (!clipboardText.empty()) {
            insertText(clipboardText);
        }
    }

    // Insert text at current cursor position
    void insertText(const std::string& text) {
        // Delete selected text if there's a selection
        if (hasSelection) {
            deleteSelectedText();
        }

        // Split the text by newlines
        size_t start = 0;
        size_t end = text.find('\n');

        // Insert first segment on current line
        lines[cursorRow].insert(cursorCol, text.substr(start, end == std::string::npos ? end : end - start));
        cursorCol += (end == std::string::npos ? text.length() : end - start);

        // Process remaining lines if any
        while (end != std::string::npos) {
            start = end + 1;
            end = text.find('\n', start);

            // Create a new line
            handleEnter();

            // Add text to the new line
            std::string segment = text.substr(start, end == std::string::npos ? end : end - start);
            lines[cursorRow].insert(cursorCol, segment);
            cursorCol += segment.length();
        }

        markDirty();
    }

    // Delete selected text
    void deleteSelectedText() {
        if (!hasSelection) return;

        // Normalize selection coordinates
        int startRow = std::min(selectionStartRow, selectionEndRow);
        int endRow = std::max(selectionStartRow, selectionEndRow);
        int startCol, endCol;

        if (startRow == endRow) {
            startCol = std::min(selectionStartCol, selectionEndCol);
            endCol = std::max(selectionStartCol, selectionEndCol);

            // Delete text on single line
            lines[startRow].erase(startCol, endCol - startCol);
        } else {
            // Handle multi-line selection
            startCol = (startRow == selectionStartRow) ? selectionStartCol : selectionEndCol;
            endCol = (endRow == selectionEndRow) ? selectionEndCol : selectionStartCol;

            // Keep the start of the first line and the end of the last line
            std::string startText = lines[startRow].substr(0, startCol);
            std::string endText = lines[endRow].substr(endCol);

            // Combine them
            lines[startRow] = startText + endText;

            // Remove the lines in between
            lines.erase(lines.begin() + startRow + 1, lines.begin() + endRow + 1);
        }

        // Move cursor to start of selection
        cursorRow = startRow;
        cursorCol = startCol;

        // Clear selection
        clearSelection();
        markDirty();
    }

    // Get selected text
    std::string getSelectedText() const {
        if (!hasSelection) return "";

        // Normalize selection coordinates
        int startRow = std::min(selectionStartRow, selectionEndRow);
        int endRow = std::max(selectionStartRow, selectionEndRow);
        int startCol, endCol;

        std::string result;

        if (startRow == endRow) {
            startCol = std::min(selectionStartCol, selectionEndCol);
            endCol = std::max(selectionStartCol, selectionEndCol);

            // Get text from single line
            result = lines[startRow].substr(startCol, endCol - startCol);
        } else {
            // Handle multi-line selection
            startCol = (startRow == selectionStartRow) ? selectionStartCol : selectionEndCol;
            endCol = (endRow == selectionEndRow) ? selectionEndCol : selectionStartCol;

            // First line
            result = lines[startRow].substr(startCol);
            result += '\n';

            // Middle lines
            for (int i = startRow + 1; i < endRow; i++) {
                result += lines[i] + '\n';
            }

            // Last line
            result += lines[endRow].substr(0, endCol);
        }

        return result;
    }

    // Find word boundaries (for Ctrl+Shift selection)
    std::pair<int, int> findWordBoundaries(int row, int col) {
        const std::string& line = lines[row];
        int start = col;
        int end = col;

        // Define what characters are part of a word
        auto isWordChar = [](char c) {
            return std::isalnum(c) || c == '_';
        };

        // Find start of word
        while (start > 0 && isWordChar(line[start - 1])) {
            start--;
        }

        // Find end of word
        while (end < static_cast<int>(line.length()) && isWordChar(line[end])) {
            end++;
        }

        return {start, end};
    }

    // Draw vertical scrollbar
    void drawVerticalScrollbar(RenderBuffer* buffer, int width, int height) {
        int totalLines = lines.size();
        int visibleLines = (height - 2 * padding) / lineHeight;

        // Only draw scrollbar if we have more lines than can be displayed
        if (totalLines <= visibleLines) return;

        // Calculate scrollbar dimensions
        int scrollbarHeight = height - 2 * padding;
        int scrollbarX = width - scrollbarWidth - padding / 2;
        int scrollbarY = padding;

        // Draw scrollbar background
        buffer->fillRectangle(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, scrollbarColor);

        // Calculate thumb position and size
        float thumbRatio = static_cast<float>(visibleLines) / totalLines;
        int thumbHeight = std::max(scrollbarMinLength, static_cast<int>(scrollbarHeight * thumbRatio));
        int thumbY = scrollbarY + static_cast<int>((scrollbarHeight - thumbHeight) *
                                                  (static_cast<float>(scrollY) / (totalLines - visibleLines)));

        // Draw thumb
        buffer->fillRectangle(scrollbarX, thumbY, scrollbarWidth, thumbHeight, scrollbarThumbColor);
    }

    // Draw horizontal scrollbar
    void drawHorizontalScrollbar(RenderBuffer* buffer, int width, int height) {
        // Find the maximum line width
        int maxLineWidth = 0;
        for (const auto& line : lines) {
            int lineWidth, textHeight, textAscent, textDescent;
            buffer->getTextExtents(font, line, lineWidth, textHeight, textAscent, textDescent);
            maxLineWidth = std::max(maxLineWidth, lineWidth);
        }

        int visibleWidth = width - 2 * padding - scrollbarWidth;

        // Only draw scrollbar if content is wider than visible area
        if (maxLineWidth <= visibleWidth) return;

        // Calculate scrollbar dimensions
        int scrollbarWidth = width - 2 * padding - this->scrollbarWidth;  // Adjust for vertical scrollbar
        int scrollbarHeight = this->scrollbarWidth;
        int scrollbarX = padding;
        int scrollbarY = height - scrollbarHeight - padding / 2;

        // Draw scrollbar background
        buffer->fillRectangle(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, scrollbarColor);

        // Calculate thumb position and size
        float thumbRatio = static_cast<float>(visibleWidth) / maxLineWidth;
        int thumbWidth = std::max(scrollbarMinLength, static_cast<int>(scrollbarWidth * thumbRatio));
        int thumbX = scrollbarX + static_cast<int>((scrollbarWidth - thumbWidth) *
                                                  (static_cast<float>(scrollX) / (maxLineWidth - visibleWidth)));

        // Draw thumb
        buffer->fillRectangle(thumbX, scrollbarY, thumbWidth, scrollbarHeight, scrollbarThumbColor);
    }
};
```

---

### Файл: `src/ui/text_field.cppm`

```cpp
module;
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <memory>
#include <iostream>

export module ui.text_field;

import ui.widget;
import ui.render_buffer;
import ui.event;

export class TextField : public Widget {
private:
    std::string text;
    std::string placeholder;
    std::unique_ptr<RenderBuffer> buffer;
    XftFont* font = nullptr;
    XftColor* textColor = nullptr;
    XftColor* placeholderColor = nullptr;

    unsigned long backgroundColor = 0xFFFFFF;  // White
    unsigned long borderColor = 0x000000;      // Black
    unsigned long focusedBorderColor = 0x0000FF; // Blue

    int cursorPosition = 0;
    bool focused = false;
    int padding = 5;  // Padding inside the text field

public:
    TextField(int x, int y, int width, int height, const std::string& placeholder = "")
        : Widget(placeholder) {  // Use placeholder as the widget ID or use an empty string
        // Set position and size manually since they're not in the constructor
        setX(x);
        setY(y);
        setWidth(width);
        setHeight(height);
        this->placeholder = placeholder;
    }

    ~TextField() {
        if (buffer) {
            if (textColor) {
                buffer->freeXftColor(textColor);
                textColor = nullptr;
            }
            if (placeholderColor) {
                buffer->freeXftColor(placeholderColor);
                placeholderColor = nullptr;
            }
        }

        // Font should be loaded and freed by the application
    }

    void setText(const std::string& newText) {
        text = newText;
        cursorPosition = text.length();
    }

    std::string getText() const {
        return text;
    }

    void setFont(XftFont* newFont) {
        font = newFont;
    }

    void setFocus(bool focus) {
        focused = focus;
    }

    bool isFocused() const {
        return focused;
    }

    void setBackgroundColor(unsigned long color) {
        backgroundColor = color;
    }

    void setBorderColor(unsigned long color) {
        borderColor = color;
    }

    void setFocusedBorderColor(unsigned long color) {
        focusedBorderColor = color;
    }

    // Добавьте этот метод перед paintToBuffer
    void handleEvent(const Event& event) override {
        // Проверяем, находится ли точка внутри текстового поля
        bool isInside = containsPoint(event.getX(), event.getY());

        if (event.getType() == Event::Type::MouseDown && isInside) {
            std::cerr << "TextField clicked, setting focus to true" << std::endl;
            setFocus(true);
            markDirty(); // Отмечаем как грязный при получении фокуса

            // Здесь можно добавить логику для установки позиции курсора
            // в зависимости от позиции клика
        }
        else if (event.getType() == Event::Type::MouseDown && !isInside) {
            std::cerr << "Click outside TextField, removing focus" << std::endl;
            setFocus(false);
            markDirty(); // Отмечаем как грязный при потере фокуса
        }

        // Обрабатываем клавиатурные события, если в фокусе
        if (focused && event.getType() == Event::Type::KeyDown) {
            std::cerr << "TextField processing key press event" << std::endl;
            XKeyEvent keyCopy = event.getNativeEvent().xkey;
            handleKeyPress(keyCopy);
            markDirty();
        }

        // Передаем событие базовому классу
        Widget::handleEvent(event);
    }

    bool handleKeyPress(XKeyEvent& event) {
        if (!focused) return false;

        char buffer[32];
        KeySym keysym;
        int count = XLookupString(&event, buffer, sizeof(buffer), &keysym, nullptr);

        if (keysym == XK_BackSpace) {
            if (cursorPosition > 0) {
                text.erase(cursorPosition - 1, 1);
                cursorPosition--;
                return true;
            }
        } else if (keysym == XK_Delete) {
            if (cursorPosition < text.length()) {
                text.erase(cursorPosition, 1);
                return true;
            }
        } else if (keysym == XK_Left) {
            if (cursorPosition > 0) {
                cursorPosition--;
                return true;
            }
        } else if (keysym == XK_Right) {
            if (cursorPosition < text.length()) {
                cursorPosition++;
                return true;
            }
        } else if (keysym == XK_Home) {
            cursorPosition = 0;
            return true;
        } else if (keysym == XK_End) {
            cursorPosition = text.length();
            return true;
        } else if (count > 0) {
            // Insert the typed character at cursor position
            text.insert(cursorPosition, buffer, count);
            cursorPosition += count;
            return true;
        }

        return false;
    }
    // Replace the render() and draw() methods with paintToBuffer()
    void paintToBuffer(Display* display) override {
        // Get the buffer from the parent class
        ensureBuffer(display, getWindow());
        RenderBuffer* buffer = getBuffer();
        if (!buffer) return;

        int width = getWidth();
        int height = getHeight();

        // Clear buffer with background color
        buffer->clear(backgroundColor);

        // Draw border
        unsigned long currentBorderColor = focused ? focusedBorderColor : borderColor;
        buffer->drawRectangle(0, 0, width - 1, height - 1, currentBorderColor);
        std::cerr << "text field paintToBuffer drawRectangle" << std::endl;

        // Initialize font if needed
        if (!font) {
            std::cerr << "Warning: No font set for TextField" << std::endl;
            return;
        }

        // Initialize colors if needed
        if (!textColor) {
            textColor = buffer->createXftColor(0, 0, 0);  // Black
        }
        if (!placeholderColor) {
            placeholderColor = buffer->createXftColor(128, 128, 128);  // Gray
        }

        // Draw text or placeholder
        std::string displayText = text.empty() ? placeholder : text;
        XftColor* currentColor = text.empty() ? placeholderColor : textColor;

        if (!displayText.empty()) {
            int textWidth, textHeight, textAscent, textDescent;
            buffer->getTextExtents(font, displayText, textWidth, textHeight, textAscent, textDescent);

            // Draw text with vertical centering
            int textY = (height + textAscent - textDescent) / 2;
            buffer->drawText(padding, textY, displayText, font, currentColor);

            // Draw cursor if focused
            if (focused && !text.empty()) {
                std::string textBeforeCursor = text.substr(0, cursorPosition);
                int cursorX;
                buffer->getTextExtents(font, textBeforeCursor, cursorX, textHeight, textAscent, textDescent);
                cursorX += padding;  // Add padding

                buffer->drawLine(cursorX, padding, cursorX, height - padding, currentBorderColor);
            } else if (focused && text.empty()) {
                // Draw cursor at beginning when text is empty
                buffer->drawLine(padding, padding, padding, height - padding, currentBorderColor);
            }
        }
    }
};
```

---

### Файл: `src/ui/widget.cppm`

```cpp
module;
#include <vector>
#include <memory>
#include <X11/Xlib.h>
#include <iostream>
#include <functional>
#include <string>
#include <algorithm>

import ui.event;
import ui.event_listener;
import ui.render_buffer;  // Import the new module

export module ui.widget;

export class Widget : public EventListener {
protected:
    bool isDirty = false;
    bool isVisible = true;
    bool isEnabled = true;
    std::string id;  // Уникальный идентификатор виджета

public:
    // Конструктор с идентификатором
    Widget(const std::string& widgetId = "") : id(widgetId) {}

    // Виртуальный деструктор для правильного удаления наследников
    virtual ~Widget() = default;

    virtual void handleEvent(const Event& event) override {
        // Обрабатываем события только если виджет включен
        if (!isEnabled) return;

        for (auto& child : children) {
            if (child->isVisible) {
                child->handleEvent(event);
            }
        }
    }

    void setPosition(int x, int y) {
        if (this->x != x || this->y != y) {
            this->x = x;
            this->y = y;
            markDirty();
        }
    }

    void setSize(unsigned int width, unsigned int height) {
        if (this->width != width || this->height != height) {
            this->width = width;
            this->height = height;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            markDirty();
        }
    }

    // Метод для установки как позиции, так и размера одновременно
    void setBounds(int x, int y, unsigned int width, unsigned int height) {
        bool changed = false;

        if (this->x != x || this->y != y) {
            this->x = x;
            this->y = y;
            changed = true;
        }

        if (this->width != width || this->height != height) {
            this->width = width;
            this->height = height;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            changed = true;
        }

        if (changed) {
            markDirty();
        }
    }

    int getX() const { return x; }
    int getY() const { return y; }
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

    // Получение абсолютных координат (с учетом родителей)
    void getAbsolutePosition(int& absX, int& absY) const {
        absX = x;
        absY = y;

        const Widget* p = parent;
        while (p) {
            absX += p->x;
            absY += p->y;
            p = p->parent;
        }
    }

    // Проверка, находится ли точка внутри виджета
    bool containsPoint(int pointX, int pointY) const {
        int absX, absY;
        getAbsolutePosition(absX, absY);

        return pointX >= absX && pointX < absX + static_cast<int>(width) &&
               pointY >= absY && pointY < absY + static_cast<int>(height);
    }

    virtual void render(Display* display, Window window) {
        if (!isVisible) return;

        setWindow(window);
        // Only render if dirty
        if (isDirty) {
            ensureBuffer(display, window);

            // Clear the buffer with the default background color
            buffer->clear(WhitePixel(display, DefaultScreen(display)));

            // Let derived classes paint their content on the buffer
            paintToBuffer(display);

            // Clear the dirty flag
            clearDirty();
        } else {
//            std::cout << "DEBUG: Widget::render - widget is not dirty, skipping buffer update" << std::endl;
        }

        // Render all children (they'll check their own dirty flags)
        for (auto& child : children) {
            child->render(display, window);
        }
    }

    // Initialize the buffer if it doesn't exist
    void ensureBuffer(Display* display, ::Window window) {
        if (!buffer) {
            buffer = std::make_unique<RenderBuffer>(display, window, width, height);
        }
    }

    virtual void paint(Display* display, Window window, GC gc) {
        if (!isVisible) return;

        // Make sure we have a buffer
        ensureBuffer(display, window);

        // Copy our buffer to the window
        buffer->copyTo(window, 0, 0, x, y, width, height);

        // Paint children
        for (auto& child : children) {
            child->paint(display, window, gc);
        }
    }

    // Paint widget content to its buffer
    virtual void paintToBuffer(Display* display) {
        // Default implementation does nothing - derived classes should override
    }

    // Get the buffer
    RenderBuffer* getBuffer() const { return buffer.get(); }

    void markDirty() {
        isDirty = true;
        // Удалено создание события перерисовки, чтобы избежать рекурсии
    }

    // Пометить как грязный этот виджет и все дочерние
    void markDirtyRecursive() {
        markDirty();
        for (auto& child : children) {
            child->markDirtyRecursive();
        }
    }

    virtual bool needsRepaint() const {
        return isDirty;
    }

    virtual void clearDirty() {
        isDirty = false;
    }

    // Set parent reference
    void setParent(Widget* parent) {
        this->parent = parent;
    }

    // Override addChild to set parent reference
    void addChild(std::shared_ptr<Widget> child) {
        child->setParent(this);
        children.push_back(std::move(child));
        markDirty(); // Добавление ребенка требует перерисовки
    }

    // Удаление дочернего виджета по указателю
    bool removeChild(Widget* childPtr) {
        auto it = std::find_if(children.begin(), children.end(),
            [childPtr](const std::shared_ptr<Widget>& child) {
                return child.get() == childPtr;
            });

        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
            markDirty();
            return true;
        }
        return false;
    }

    // Удаление дочернего виджета по ID
    bool removeChildById(const std::string& childId) {
        auto it = std::find_if(children.begin(), children.end(),
            [&childId](const std::shared_ptr<Widget>& child) {
                return child->getId() == childId;
            });

        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
            markDirty();
            return true;
        }
        return false;
    }

    // Очистка всех дочерних виджетов
    void clearChildren() {
        for (auto& child : children) {
            child->setParent(nullptr);
        }
        children.clear();
        markDirty();
    }

    std::vector<Widget*> getChildren() const {
        std::vector<Widget*> rawPointers;
        for (const auto& child : children) {
            rawPointers.push_back(child.get());
        }
        return rawPointers;
    }

    // Поиск дочернего виджета по ID (включая вложенные)
    Widget* findChildById(const std::string& childId) {
        if (id == childId) return this;

        for (auto& child : children) {
            Widget* found = child->findChildById(childId);
            if (found) return found;
        }

        return nullptr;
    }

    // Метод для установки текущего окна
    void setWindow(Window window) {
        currentWindow = window;
    }

    // Метод для получения текущего окна
    Window getWindow() const {
        return currentWindow;
    }

    // Методы для управления видимостью
    void setVisible(bool visible) {
        if (isVisible != visible) {
            isVisible = visible;
            markDirty();
        }
    }

    bool getVisible() const { return isVisible; }

    // Методы для управления активностью
    void setEnabled(bool enabled) {
        if (isEnabled != enabled) {
            isEnabled = enabled;
            markDirty();
        }
    }

    bool getEnabled() const { return isEnabled; }

    // Методы для работы с ID
    void setId(const std::string& newId) { id = newId; }
    const std::string& getId() const { return id; }

    void setX(int newX) {
        if (this->x != newX) {
            this->x = newX;
            markDirty();
        }
    }

    void setY(int newY) {
        if (this->y != newY) {
            this->y = newY;
            markDirty();
        }
    }

    void setWidth(unsigned int newWidth) {
        if (this->width != newWidth) {
            this->width = newWidth;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            markDirty();
        }
    }

    void setHeight(unsigned int newHeight) {
        if (this->height != newHeight) {
            this->height = newHeight;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            markDirty();
        }
    }

    // Получение родительского виджета
    Widget* getParent() const { return parent; }

private:
    Widget* parent = nullptr;  // Parent reference for propagating dirty state
    std::vector<std::shared_ptr<Widget>> children;
    int x = 0;
    int y = 0;
    unsigned int width = 100;
    unsigned int height = 30;
    std::unique_ptr<RenderBuffer> buffer;  // Add buffer for off-screen rendering
    Window currentWindow = 0;  // Добавляем поле для хранения окна
};
```

---

