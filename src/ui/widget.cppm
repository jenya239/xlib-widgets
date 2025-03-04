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