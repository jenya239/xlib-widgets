# Исходные файлы проекта

## Краткая статистика

- Всего файлов: 13
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
// main.cpp
#include <X11/Xlib.h>
#include <memory>
#include <iostream>
#include <functional>
#include <cstring>
#include <typeinfo>

// Для использования C++20 модулей в обычном .cpp файле нужно использовать import
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import services.render_service;
import ui.application;
import ui.application_window;
import ui.button;
import ui.event;
import ui.widget;

int main() {
    try {
        // Создаем и инициализируем приложение
        auto app = std::make_shared<Application>();

        // Создаем главное окно
        if (!app->createMainWindow("My Application", 800, 600)) {
            return 1;
        }

        auto mainWindow = app->getMainWindow();
        auto logger = app->getLogger();

        // Создаем кнопку
        auto button = std::make_shared<Button>("Click Me!");
        button->setPosition(100, 100);
        button->setSize(200, 50);

        // Настраиваем обработчики событий для кнопки
        button->onClick = [&logger]() {
            logger->info("Button clicked!");
        };

        button->onMouseEnter = [&logger, button]() {
            logger->info("Mouse entered button: " + button->getLabel());
            logger->debug("Button state changed to HOVER");
        };

        button->onMouseLeave = [&logger, button]() {
            logger->info("Mouse left button: " + button->getLabel());
            logger->debug("Button state changed to NORMAL");
        };

        button->onMouseDown = [&logger]() {
            logger->debug("Button state changed to PRESSED");
        };

        button->onMouseUp = [&logger]() {
            logger->debug("Button state changed to HOVER (after press)");
        };

        // Добавляем кнопку в окно
        mainWindow->addChild(button);

        // Настраиваем обработку событий
        app->setupEventHandling();

        // Выполняем первоначальную отрисовку
        app->initialRender();

        // Запускаем цикл обработки событий
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
        XSelectInput(display, xWindow,
            ExposureMask | ButtonPressMask | ButtonReleaseMask |
            KeyPressMask | StructureNotifyMask | PointerMotionMask);

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
#include <functional>
#include <iostream>

import ui.widget;
import ui.event;
import ui.event_listener;
import ui.render_buffer;

export module ui.button;

// Define button states
export enum class ButtonState {
    Normal,
    Hover,
    Pressed
};

export class Button : public Widget {
private:
    std::string label;
    ButtonState state = ButtonState::Normal;  // Current button state
    bool isMouseOver = false;  // Флаг для отслеживания нахождения мыши над кнопкой

    // Проверяет, находится ли точка внутри кнопки
    bool isPointInside(int x, int y) const {
        return x >= getX() && x < getX() + getWidth() &&
               y >= getY() && y < getY() + getHeight();
    }

public:
    explicit Button(std::string label) : label(std::move(label)) {}

    void handleEvent(const Event& event) override {
        // Проверяем, находится ли мышь внутри кнопки
        bool isInside = isPointInside(event.getX(), event.getY());

        if (event.getType() == Event::Type::MouseMove) {
            // Обработка входа/выхода мыши
            if (isInside && !isMouseOver) {
                // Мышь вошла в область кнопки
                isMouseOver = true;
                setState(ButtonState::Hover);
                std::cout << "DEBUG: Mouse entered button - setting state to Hover" << std::endl;
                if (onMouseEnter) {
                    onMouseEnter();
                }
            }
            else if (!isInside && isMouseOver) {
                // Мышь вышла из области кнопки
                isMouseOver = false;
                setState(ButtonState::Normal);
                std::cout << "DEBUG: Mouse left button - setting state to Normal" << std::endl;
                if (onMouseLeave) {
                    onMouseLeave();
                }
            }
            // Убираем лишний лог для MouseMove
        }
        else if (event.getType() == Event::Type::MouseDown && isInside) {
            // Нажатие кнопки мыши внутри кнопки
            setState(ButtonState::Pressed);
            std::cout << "DEBUG: Mouse down on button - setting state to Pressed" << std::endl;
            if (onMouseDown) {
                onMouseDown();
            }
        }
        else if (event.getType() == Event::Type::MouseUp) {
            // Отпускание кнопки мыши
            if (state == ButtonState::Pressed) {
                if (isInside) {
                    // Если мышь внутри кнопки, вызываем onClick и меняем состояние на Hover
                    setState(ButtonState::Hover);
                    std::cout << "DEBUG: Mouse up inside button - setting state to Hover" << std::endl;
                    if (onClick) {
                        onClick();
                    }
                    if (onMouseUp) {
                        onMouseUp();
                    }
                } else {
                    // Если мышь вне кнопки, просто меняем состояние на Normal
                    setState(ButtonState::Normal);
                    std::cout << "DEBUG: Mouse up outside button - setting state to Normal" << std::endl;
                    if (onMouseUp) {
                        onMouseUp();
                    }
                }
            }
        }

        // Передаем событие дальше базовому классу для обработки дочерними виджетами
        Widget::handleEvent(event);
    }

    // Set button state and mark as dirty if state changed
    void setState(ButtonState newState) {
        if (state != newState) {
            ButtonState oldState = state;
            state = newState;
            std::cout << "DEBUG: Button state changed from " << static_cast<int>(oldState)
                      << " to " << static_cast<int>(newState) << " - marking as dirty" << std::endl;
            markDirty();  // Вызываем метод из базового класса Widget

            // Явно запрашиваем перерисовку
            std::cout << "DEBUG: needsRepaint() returns: " << (needsRepaint() ? "true" : "false") << std::endl;
        }
    }

    // Override paintToBuffer to change appearance based on state
    void paintToBuffer(Display* display) override {
        std::cout << "DEBUG: Button::paintToBuffer called with state: " << static_cast<int>(state) << std::endl;

        RenderBuffer* buffer = getBuffer();
        if (!buffer) {
            std::cout << "ERROR: Buffer is null in paintToBuffer! Creating new buffer." << std::endl;
            ensureBuffer(display, getWindow());
            buffer = getBuffer();
            if (!buffer) {
                std::cout << "ERROR: Failed to create buffer!" << std::endl;
                return;
            }
        }

        GC gc = buffer->getGC();
        Pixmap pixmap = buffer->getPixmap();

        std::cout << "DEBUG: Buffer GC: " << gc << ", Pixmap: " << pixmap << std::endl;

        // Остальной код без изменений...
        unsigned long bgColor;
        switch (state) {
            case ButtonState::Pressed:
                bgColor = 0x777777;  // Темно-серый при нажатии
                std::cout << "DEBUG: Using PRESSED color: 0x777777" << std::endl;
                break;
            case ButtonState::Hover:
                bgColor = 0xAAAAAA;  // Светло-серый при наведении
                std::cout << "DEBUG: Using HOVER color: 0xAAAAAA" << std::endl;
                break;
            case ButtonState::Normal:
            default:
                bgColor = 0xCCCCCC;  // Обычный серый
                std::cout << "DEBUG: Using NORMAL color: 0xCCCCCC" << std::endl;
                break;
        }
        unsigned long textColor = 0x000000;  // Черный текст

        // Преобразуем цвета в формат X11
        XColor xcolor;
        Colormap colormap = DefaultColormap(display, DefaultScreen(display));

        // Устанавливаем цвет фона
        xcolor.red = ((bgColor >> 16) & 0xFF) * 256;
        xcolor.green = ((bgColor >> 8) & 0xFF) * 256;
        xcolor.blue = (bgColor & 0xFF) * 256;
        xcolor.flags = DoRed | DoGreen | DoBlue;

        if (XAllocColor(display, colormap, &xcolor)) {
            std::cout << "DEBUG: Allocated background color: " << xcolor.pixel << std::endl;
            XSetForeground(display, gc, xcolor.pixel);
        } else {
            std::cout << "ERROR: Failed to allocate background color!" << std::endl;
            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
        }

        // Draw button background
        XFillRectangle(display, pixmap, gc, 0, 0, getWidth(), getHeight());

        // Устанавливаем цвет границы (черный)
        XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
        XDrawRectangle(display, pixmap, gc, 0, 0, getWidth() - 1, getHeight() - 1);

        // Text position adjustment for pressed state (slight shift down/right)
        int textX = (state == ButtonState::Pressed) ? 11 : 10;
        int textY = (state == ButtonState::Pressed) ? getHeight() / 2 + 6 : getHeight() / 2 + 5;

        // Draw button text
        XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
        XDrawString(display, pixmap, gc,
                    textX, textY,
                    label.c_str(), label.length());

        std::cout << "DEBUG: Button painted with state: " << static_cast<int>(state) << std::endl;
    }

    // Keep the old paint method for compatibility, but it will now use the buffer
    void paint(Display* display, ::Window window, GC gc) override {
//        std::cout << "DEBUG: Button::paint called" << std::endl;

        // Вызываем render, чтобы обновить буфер, если нужно
        render(display, window);

        // Теперь вызываем базовый метод, который скопирует буфер на экран
        Widget::paint(display, window, gc);
    }

    // Переопределяем метод needsRepaint для отладки
    bool needsRepaint() const override {
        bool result = Widget::needsRepaint();
        return result;
    }

    // Переопределяем метод clearDirty для отладки
    void clearDirty() override {
        std::cout << "DEBUG: Button::clearDirty() called" << std::endl;
        Widget::clearDirty();
    }

    // Геттер для получения текста кнопки
    std::string getLabel() const {
        return label;
    }

    // Колбэки для различных событий
    std::function<void()> onClick;
    std::function<void()> onMouseEnter;
    std::function<void()> onMouseLeave;
    std::function<void()> onMouseDown;  // Добавлен новый колбэк
    std::function<void()> onMouseUp;    // Добавлен новый колбэк
};
```

---

### Файл: `src/ui/event.cppm`

```cpp
module;
#include <X11/Xlib.h>

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
                type = Type::MouseDown;
                x = xEvent.xbutton.x;
                y = xEvent.xbutton.y;
                button = xEvent.xbutton.button;
                break;
            case ButtonRelease:
                type = Type::MouseUp;
                x = xEvent.xbutton.x;
                y = xEvent.xbutton.y;
                button = xEvent.xbutton.button;
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
// src/ui/render_buffer.cppm
module;
#include <X11/Xlib.h>
#include <memory>
#include <stdexcept>
#include <iostream>

export module ui.render_buffer;

export class RenderBuffer {
private:
    Display* display;
    Pixmap pixmap;
    GC gc;
    unsigned int width;
    unsigned int height;
    bool valid = true;  // Флаг для проверки валидности буфера

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

        // Clear the pixmap initially
        XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
        XFillRectangle(display, pixmap, gc, 0, 0, width, height);
    }

    ~RenderBuffer() {
        if (valid) {
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
          width(other.width), height(other.height), valid(other.valid) {
        other.pixmap = 0;
        other.gc = nullptr;
        other.valid = false;
    }

    RenderBuffer& operator=(RenderBuffer&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (valid) {
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

            // Invalidate other
            other.pixmap = 0;
            other.gc = nullptr;
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

### Файл: `src/ui/widget.cppm`

```cpp
module;
#include <vector>
#include <memory>
#include <X11/Xlib.h>
#include <iostream>

import ui.event;
import ui.event_listener;
import ui.render_buffer;  // Import the new module

export module ui.widget;

export class Widget : public EventListener {
protected:
    bool isDirty = false;

public:
    virtual void handleEvent(const Event& event) override {
        for (auto& child : children) {
            child->handleEvent(event);
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

    int getX() const { return x; }
    int getY() const { return y; }
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

    virtual void render(Display* display, Window window) {
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
    }

    std::vector<Widget*> getChildren() const {
        std::vector<Widget*> rawPointers;
        for (const auto& child : children) {
            rawPointers.push_back(child.get());
        }
        return rawPointers;
    }

    // Метод для установки текущего окна
    void setWindow(Window window) {
        currentWindow = window;
    }

    // Метод для получения текущего окна
    Window getWindow() const {
        return currentWindow;
    }

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

