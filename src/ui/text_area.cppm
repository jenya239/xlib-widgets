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

        // Check for Ctrl key
        bool ctrlPressed = (event.state & ControlMask);

        // Handle copy/paste shortcuts
        if (ctrlPressed) {
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
            currentBuffer->drawLine(padding, padding, padding, height - padding, currentBorderColor);
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
};