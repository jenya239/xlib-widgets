// src/ui/render_buffer.cppm
module;
#include <X11/Xlib.h>
#include <memory>
#include <stdexcept>
#include <iostream>

export module ui.render_buffer;

export class RenderBuffer {
private:
    Display* display;
    Pixmap pixmap;
    GC gc;
    unsigned int width;
    unsigned int height;
    bool valid = true;  // Флаг для проверки валидности буфера

public:
    RenderBuffer(Display* display, ::Window window, unsigned int width, unsigned int height)
        : display(display), width(width), height(height) {

        if (!display) {
            throw std::invalid_argument("Display pointer cannot be null");
        }

        if (width == 0 || height == 0) {
            throw std::invalid_argument("Width and height must be greater than zero");
        }

        // Create an off-screen pixmap
        pixmap = XCreatePixmap(display, window, width, height,
                              DefaultDepth(display, DefaultScreen(display)));

        if (!pixmap) {
            throw std::runtime_error("Failed to create pixmap");
        }

        // Create a GC for drawing on this pixmap
        gc = XCreateGC(display, pixmap, 0, nullptr);

        if (!gc) {
            XFreePixmap(display, pixmap);
            throw std::runtime_error("Failed to create graphics context");
        }

        // Clear the pixmap initially
        XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
        XFillRectangle(display, pixmap, gc, 0, 0, width, height);
    }

    ~RenderBuffer() {
        if (valid) {
            XFreeGC(display, gc);
            XFreePixmap(display, pixmap);
        }
    }

    // No copying
    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;

    // Moving is allowed
    RenderBuffer(RenderBuffer&& other) noexcept
        : display(other.display), pixmap(other.pixmap), gc(other.gc),
          width(other.width), height(other.height), valid(other.valid) {
        other.pixmap = 0;
        other.gc = nullptr;
        other.valid = false;
    }

    RenderBuffer& operator=(RenderBuffer&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (valid) {
                XFreeGC(display, gc);
                XFreePixmap(display, pixmap);
            }

            // Move resources from other
            display = other.display;
            pixmap = other.pixmap;
            gc = other.gc;
            width = other.width;
            height = other.height;
            valid = other.valid;

            // Invalidate other
            other.pixmap = 0;
            other.gc = nullptr;
            other.valid = false;
        }
        return *this;
    }

    // Get the pixmap for blitting
    Pixmap getPixmap() const { return pixmap; }

    // Get the GC for drawing
    GC getGC() const { return gc; }

    // Clear the buffer
    void clear(unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XFillRectangle(display, pixmap, gc, 0, 0, width, height);
    }

    void copyTo(Drawable dest, int srcX, int srcY, int destX, int destY,
                unsigned int width, unsigned int height) {
        if (!valid) {
            std::cout << "DEBUG: RenderBuffer::copyTo - Buffer is not valid!" << std::endl;
            return;
        }

//        std::cout << "DEBUG: RenderBuffer::copyTo - Copying from (" << srcX << "," << srcY
//                  << ") to (" << destX << "," << destY << ") with size "
//                  << width << "x" << height << std::endl;

        // Проверка границ
        if (srcX < 0) srcX = 0;
        if (srcY < 0) srcY = 0;
        if (srcX + width > this->width) width = this->width - srcX;
        if (srcY + height > this->height) height = this->height - srcY;

        if (width > 0 && height > 0) {
//            std::cout << "DEBUG: RenderBuffer::copyTo - After bounds check: from (" << srcX << "," << srcY
//                      << ") with size " << width << "x" << height << std::endl;

            // Создаем временный GC для операции копирования
            GC tempGC = XCreateGC(display, dest, 0, nullptr);
            if (tempGC) {
//                std::cout << "DEBUG: RenderBuffer::copyTo - Using temporary GC" << std::endl;
                XCopyArea(display, pixmap, dest, tempGC, srcX, srcY, width, height, destX, destY);
                XFreeGC(display, tempGC);
            } else {
//                std::cout << "DEBUG: RenderBuffer::copyTo - Using buffer's GC" << std::endl;
                XCopyArea(display, pixmap, dest, gc, srcX, srcY, width, height, destX, destY);
            }
            XFlush(display);
        } else {
            std::cout << "DEBUG: RenderBuffer::copyTo - Nothing to copy after bounds check" << std::endl;
        }
    }

    // Упрощенная версия copyTo для копирования всего буфера
    void copyToWindow(::Window window, int x, int y) {
        copyTo(window, 0, 0, x, y, width, height);
    }

    // Проверка валидности буфера
    bool isValid() const { return valid; }

    // Изменение размера буфера
    bool resize(::Window window, unsigned int newWidth, unsigned int newHeight) {
        if (!valid || newWidth == 0 || newHeight == 0) return false;

        // Создаем новый pixmap
        Pixmap newPixmap = XCreatePixmap(display, window, newWidth, newHeight,
                                        DefaultDepth(display, DefaultScreen(display)));
        if (!newPixmap) return false;

        // Копируем содержимое старого pixmap в новый
        unsigned int copyWidth = std::min(width, newWidth);
        unsigned int copyHeight = std::min(height, newHeight);
        XCopyArea(display, pixmap, newPixmap, gc, 0, 0, copyWidth, copyHeight, 0, 0);

        // Освобождаем старый pixmap
        XFreePixmap(display, pixmap);

        // Обновляем данные
        pixmap = newPixmap;
        width = newWidth;
        height = newHeight;

        // Очищаем новые области, если буфер увеличился
        if (newWidth > copyWidth) {
            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
            XFillRectangle(display, pixmap, gc, copyWidth, 0, newWidth - copyWidth, copyHeight);
        }

        if (newHeight > copyHeight) {
            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
            XFillRectangle(display, pixmap, gc, 0, copyHeight, newWidth, newHeight - copyHeight);
        }

        return true;
    }
    
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }
};