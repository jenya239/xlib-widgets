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