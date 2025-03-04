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