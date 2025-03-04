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