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