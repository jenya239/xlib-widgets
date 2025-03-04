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