module;
#include <vector>
#include <memory>
#include <X11/Xlib.h>
#include <iostream>
#include <functional>
#include <string>
#include <algorithm>

import ui.event;
import ui.event_listener;
import ui.render_buffer;  // Import the new module

export module ui.widget;

export class Widget : public EventListener {
protected:
    bool isDirty = false;
    bool isVisible = true;
    bool isEnabled = true;
    std::string id;  // Уникальный идентификатор виджета

public:
    // Конструктор с идентификатором
    Widget(const std::string& widgetId = "") : id(widgetId) {}

    // Виртуальный деструктор для правильного удаления наследников
    virtual ~Widget() = default;

    virtual void handleEvent(const Event& event) override {
        // Обрабатываем события только если виджет включен
        if (!isEnabled) return;

        for (auto& child : children) {
            if (child->isVisible) {
                child->handleEvent(event);
            }
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

    // Метод для установки как позиции, так и размера одновременно
    void setBounds(int x, int y, unsigned int width, unsigned int height) {
        bool changed = false;

        if (this->x != x || this->y != y) {
            this->x = x;
            this->y = y;
            changed = true;
        }

        if (this->width != width || this->height != height) {
            this->width = width;
            this->height = height;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            changed = true;
        }

        if (changed) {
            markDirty();
        }
    }

    int getX() const { return x; }
    int getY() const { return y; }
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }

    // Получение абсолютных координат (с учетом родителей)
    void getAbsolutePosition(int& absX, int& absY) const {
        absX = x;
        absY = y;

        const Widget* p = parent;
        while (p) {
            absX += p->x;
            absY += p->y;
            p = p->parent;
        }
    }

    // Проверка, находится ли точка внутри виджета
    bool containsPoint(int pointX, int pointY) const {
        int absX, absY;
        getAbsolutePosition(absX, absY);

        return pointX >= absX && pointX < absX + static_cast<int>(width) &&
               pointY >= absY && pointY < absY + static_cast<int>(height);
    }

    virtual void render(Display* display, Window window) {
        if (!isVisible) return;

        setWindow(window);
        // Only render if dirty
        if (isDirty) {
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
        if (!isVisible) return;

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

    // Пометить как грязный этот виджет и все дочерние
    void markDirtyRecursive() {
        markDirty();
        for (auto& child : children) {
            child->markDirtyRecursive();
        }
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
        markDirty(); // Добавление ребенка требует перерисовки
    }

    // Удаление дочернего виджета по указателю
    bool removeChild(Widget* childPtr) {
        auto it = std::find_if(children.begin(), children.end(),
            [childPtr](const std::shared_ptr<Widget>& child) {
                return child.get() == childPtr;
            });

        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
            markDirty();
            return true;
        }
        return false;
    }

    // Удаление дочернего виджета по ID
    bool removeChildById(const std::string& childId) {
        auto it = std::find_if(children.begin(), children.end(),
            [&childId](const std::shared_ptr<Widget>& child) {
                return child->getId() == childId;
            });

        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
            markDirty();
            return true;
        }
        return false;
    }

    // Очистка всех дочерних виджетов
    void clearChildren() {
        for (auto& child : children) {
            child->setParent(nullptr);
        }
        children.clear();
        markDirty();
    }

    std::vector<Widget*> getChildren() const {
        std::vector<Widget*> rawPointers;
        for (const auto& child : children) {
            rawPointers.push_back(child.get());
        }
        return rawPointers;
    }

    // Поиск дочернего виджета по ID (включая вложенные)
    Widget* findChildById(const std::string& childId) {
        if (id == childId) return this;

        for (auto& child : children) {
            Widget* found = child->findChildById(childId);
            if (found) return found;
        }

        return nullptr;
    }

    // Метод для установки текущего окна
    void setWindow(Window window) {
        currentWindow = window;
    }

    // Метод для получения текущего окна
    Window getWindow() const {
        return currentWindow;
    }

    // Методы для управления видимостью
    void setVisible(bool visible) {
        if (isVisible != visible) {
            isVisible = visible;
            markDirty();
        }
    }

    bool getVisible() const { return isVisible; }

    // Методы для управления активностью
    void setEnabled(bool enabled) {
        if (isEnabled != enabled) {
            isEnabled = enabled;
            markDirty();
        }
    }

    bool getEnabled() const { return isEnabled; }

    // Методы для работы с ID
    void setId(const std::string& newId) { id = newId; }
    const std::string& getId() const { return id; }

    void setX(int newX) {
        if (this->x != newX) {
            this->x = newX;
            markDirty();
        }
    }

    void setY(int newY) {
        if (this->y != newY) {
            this->y = newY;
            markDirty();
        }
    }

    virtual void setWidth(unsigned int newWidth) {
        if (this->width != newWidth) {
            this->width = newWidth;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            markDirty();
        }
    }

    virtual void setHeight(unsigned int newHeight) {
        if (this->height != newHeight) {
            this->height = newHeight;

            // Recreate buffer if size changes
            if (buffer) {
                buffer.reset();
            }

            markDirty();
        }
    }

    // Получение родительского виджета
    Widget* getParent() const { return parent; }

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