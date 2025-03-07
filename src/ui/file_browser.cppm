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
    bool focused = false;

    std::function<void(const std::filesystem::path&)> onFileSelected;

    // Scrollbar constants
    const int scrollbarWidth = 10;
    const int scrollbarMinLength = 20;

    // For double-click detection
    Time lastClickTime = 0;

public:
    FileBrowser(const std::string& widgetId = "")
        : Widget(widgetId), currentPath(std::filesystem::current_path()) {
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

        if (event.getType() == Event::Type::MouseDown && isInside) {
            focused = true;

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

            if (entry.isDirectory) {
                // Navigate to directory
                if (entry.isParentDir) {
                    setPath(currentPath.parent_path());
                } else {
                    setPath(currentPath / entry.name);
                }
            } else if (onFileSelected) {
                // Call the callback with the selected file path
                onFileSelected(currentPath / entry.name);
            }
        }
    }

    void ensureSelectedVisible() {
        int visibleLines = (getHeight() - 2 * padding) / lineHeight;

        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        } else if (selectedIndex >= scrollOffset + visibleLines) {
            scrollOffset = selectedIndex - visibleLines + 1;
        }

        if (scrollOffset < 0) scrollOffset = 0;
    }

    void paintToBuffer(Display* display) override {
        ensureBuffer(display, getWindow());
        RenderBuffer* currentBuffer = getBuffer();
        if (!currentBuffer || !font) return;

        int width = getWidth();
        int height = getHeight();

        // Clear background
        currentBuffer->clear(backgroundColor);

        // Draw border
        currentBuffer->drawRectangle(0, 0, width - 1, height - 1, borderColor);

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