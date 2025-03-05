# Исходные файлы проекта

## Краткая статистика

- Всего файлов: 14
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
src/ui/
  └── application.cppm
  └── application_window.cppm
  └── button.cppm
  └── event.cppm
  └── event_listener.cppm
  └── render_buffer.cppm
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

// Import necessary modules
import core.ioc_container;
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import services.render_service;
import ui.application;
import ui.button;
import ui.text_field;
import ui.event;
import ui.event_listener;

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

        // Create a text field with more visible properties
        auto textField = std::make_shared<TextField>(50, 120, 250, 30, "Type here...");
        textField->setVisible(true);  // Ensure visibility is set

        // Make sure the text field needs repainting
        textField->markDirty();

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

        // Create a custom event listener for text field focus
        class TextFieldEventListener : public EventListener {
        private:
            std::shared_ptr<TextField> textField;
            std::shared_ptr<LoggerService> logger;

        public:
            TextFieldEventListener(std::shared_ptr<TextField> tf, std::shared_ptr<LoggerService> log)
                : textField(tf), logger(log) {}

            bool handleEvent(const Event& event) override {
                // Get event type using the appropriate method
                Event::Type eventType = event.getType();

                // Handle mouse button press for focus management
                if (eventType == Event::BUTTON_PRESS) {
                    // Get mouse coordinates from the event
                    int x = event.getX();
                    int y = event.getY();

                    // Check if click is within text field bounds
                    if (x >= textField->getX() && x <= textField->getX() + textField->getWidth() &&
                        y >= textField->getY() && y <= textField->getY() + textField->getHeight()) {
                        textField->setFocus(true);
                        logger->info("TextField focused");
                        return true;
                    } else if (textField->isFocused()) {
                        textField->setFocus(false);
                        logger->info("TextField lost focus");
                    }
                }
                // Handle key press events when text field is focused
                else if (eventType == Event::KEY_PRESS && textField->isFocused()) {
                    // Get key information from the event
                    char key = event.getKeyChar();
                    textField->addCharacter(key);
                    logger->info("Key pressed in TextField: " + std::string(1, key));
                    return true;
                }
                return false;
            }
        };

        // Create the event listener
        auto textFieldListener = std::make_shared<TextFieldEventListener>(textField, logger);

        // Register the event listener with the main window
        mainWindow->addEventHandler(textFieldListener);

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
    int width = 800;
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
                              PointerMotionMask | EnterWindowMask | LeaveWindowMask);

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

    // Widget implementation
    void render(Display* display, Window window) override {
        int width = getWidth();
        int height = getHeight();

        // Resize buffer if needed
        if (!buffer || buffer->getWidth() != width || buffer->getHeight() != height) {
            buffer = std::make_unique<RenderBuffer>(display, window, width, height);
        }

        // Clear buffer with background color
        buffer->clear(backgroundColor);

        // Draw border
        unsigned long currentBorderColor = focused ? focusedBorderColor : borderColor;
        buffer->drawRectangle(0, 0, width - 1, height - 1, currentBorderColor);

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

    void draw(Display* display, Window window) {
        render(display, window);
        if (buffer) {
            buffer->copyToWindow(window, getX(), getY());
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
            std::cout << "DEBUG: Widget::render - widget is dirty, updating buffer" << std::endl;
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

