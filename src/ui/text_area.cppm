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
    std::unique_ptr<RenderBuffer> buffer;
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
            markDirty(); // Mark as dirty when gaining focus

            // TODO: Add logic to set cursor position based on click position
        }
        else if (event.getType() == Event::Type::MouseDown && !isInside) {
            std::cerr << "Click outside TextArea, removing focus" << std::endl;
            setFocus(false);
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

    bool handleKeyPress(XKeyEvent& event) {
        if (!focused) return false;

        char buffer[32];
        KeySym keysym;
        int count = XLookupString(&event, buffer, sizeof(buffer), &keysym, nullptr);

        bool result = false;

        // Handle basic navigation keys
        if (keysym == XK_BackSpace) {
            result = handleBackspace();
        } else if (keysym == XK_Delete) {
            result = handleDelete();
        } else if (keysym == XK_Left) {
            result = handleLeftArrow();
        } else if (keysym == XK_Right) {
            result = handleRightArrow();
        } else if (keysym == XK_Up) {
            result = handleUpArrow();
        } else if (keysym == XK_Down) {
            result = handleDownArrow();
        } else if (keysym == XK_Home) {
            cursorCol = 0;
            result = true;
        } else if (keysym == XK_End) {
            cursorCol = lines[cursorRow].length();
            result = true;
        } else if (keysym == XK_Return || keysym == XK_KP_Enter) {
            result = handleEnter();
        } else if (keysym == XK_Tab) {
            result = handleTab();
        }  else if (count > 0) {
            // Insert the typed character at cursor position
            lines[cursorRow].insert(cursorCol, buffer, count);
            cursorCol += count;
            result = true;
        }

        if (result) {
            ensureCursorVisible();
        }

        return false;
    }

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
        std::cerr << "text area paintToBuffer drawRectangle" << std::endl;

        // Initialize font if needed
        if (!font) {
            std::cerr << "Warning: No font set for TextArea" << std::endl;
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
        bool isEmpty = lines.size() == 1 && lines[0].empty();
        XftColor* currentColor = isEmpty ? placeholderColor : textColor;

        if (isEmpty && !placeholder.empty()) {
            // Draw placeholder text
            int textWidth, textHeight, textAscent, textDescent;
            buffer->getTextExtents(font, placeholder, textWidth, textHeight, textAscent, textDescent);

            // Draw text with vertical centering for placeholder
            int textY = (height + textAscent - textDescent) / 2;
            buffer->drawText(padding, textY, placeholder, font, currentColor);

            // Draw cursor at beginning if focused
            if (focused) {
                buffer->drawLine(padding, padding, padding, height - padding, currentBorderColor);
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

                // Draw the line
                buffer->drawText(padding - scrollX, y, line, font, textColor);

                // Draw cursor if this is the current line and we're focused
                if (focused && static_cast<int>(i) == cursorRow) {
                    std::string textBeforeCursor = line.substr(0, cursorCol);
                    int cursorX, textHeight, textAscent, textDescent;
                    buffer->getTextExtents(font, textBeforeCursor, cursorX, textHeight, textAscent, textDescent);
                    cursorX += padding - scrollX;  // Add padding and adjust for scroll

                    buffer->drawLine(cursorX, y - font->ascent, cursorX, y + font->descent, currentBorderColor);
                }

                y += lineHeight;
            }
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
            if (buffer) {
                buffer->getTextExtents(font, textBeforeCursor, textWidth, textHeight, textAscent, textDescent);

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
};