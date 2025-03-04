module;
#include <X11/Xlib.h>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <string>
#include <X11/Xft/Xft.h>

export module ui.render_buffer;

export class RenderBuffer {
private:
    Display* display;
    Pixmap pixmap;
    GC gc;
    unsigned int width;
    unsigned int height;
    bool valid = true;  // Флаг для проверки валидности буфера
    XftDraw* xftDraw = nullptr;

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

        // Create XftDraw for text rendering
        Visual* visual = DefaultVisual(display, DefaultScreen(display));
        Colormap colormap = DefaultColormap(display, DefaultScreen(display));
        xftDraw = XftDrawCreate(display, pixmap, visual, colormap);

        if (!xftDraw) {
            XFreeGC(display, gc);
            XFreePixmap(display, pixmap);
            throw std::runtime_error("Failed to create Xft draw context");
        }

        // Clear the pixmap initially
        XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
        XFillRectangle(display, pixmap, gc, 0, 0, width, height);
    }

    ~RenderBuffer() {
        if (valid) {
            if (xftDraw) {
                XftDrawDestroy(xftDraw);
            }
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
          width(other.width), height(other.height), valid(other.valid),
          xftDraw(other.xftDraw) {
        other.pixmap = 0;
        other.gc = nullptr;
        other.xftDraw = nullptr;
        other.valid = false;
    }

    RenderBuffer& operator=(RenderBuffer&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (valid) {
                if (xftDraw) {
                    XftDrawDestroy(xftDraw);
                }
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
            xftDraw = other.xftDraw;

            // Invalidate other
            other.pixmap = 0;
            other.gc = nullptr;
            other.xftDraw = nullptr;
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

    // Добавленные методы для рисования

    // Рисование линии
    void drawLine(int x1, int y1, int x2, int y2, unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XDrawLine(display, pixmap, gc, x1, y1, x2, y2);
    }

    // Рисование прямоугольника (контур)
    void drawRectangle(int x, int y, unsigned int width, unsigned int height, unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XDrawRectangle(display, pixmap, gc, x, y, width, height);
    }

    // Рисование закрашенного прямоугольника
    void fillRectangle(int x, int y, unsigned int width, unsigned int height, unsigned long color) {
        if (!valid) return;

        XSetForeground(display, gc, color);
        XFillRectangle(display, pixmap, gc, x, y, width, height);
    }

    // Рисование текста с использованием Xft
    void drawText(int x, int y, const std::string& text, XftFont* font, XftColor* color) {
        if (!valid || !xftDraw || !font || !color) return;

        XftDrawString8(xftDraw, color, font, x, y,
                      (const FcChar8*)text.c_str(), text.length());
    }

    // Создание цвета Xft из RGB
    XftColor* createXftColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        XftColor* color = new XftColor();
        XRenderColor renderColor;
        renderColor.red = r * 257;    // Convert 0-255 to 0-65535
        renderColor.green = g * 257;
        renderColor.blue = b * 257;
        renderColor.alpha = a * 257;

        Colormap colormap = DefaultColormap(display, DefaultScreen(display));
        XftColorAllocValue(display, DefaultVisual(display, DefaultScreen(display)),
                          colormap, &renderColor, color);

        return color;
    }

    // Освобождение цвета Xft
    void freeXftColor(XftColor* color) {
        if (color) {
            Colormap colormap = DefaultColormap(display, DefaultScreen(display));
            XftColorFree(display, DefaultVisual(display, DefaultScreen(display)),
                        colormap, color);
            delete color;
        }
    }

    // Получение размеров текста
    void getTextExtents(XftFont* font, const std::string& text,
                        int& width, int& height, int& ascent, int& descent) {
        if (!font) {
            width = height = ascent = descent = 0;
            return;
        }

        XGlyphInfo extents;
        XftTextExtents8(display, font, (const FcChar8*)text.c_str(), text.length(), &extents);

        width = extents.width;
        height = extents.height;
        ascent = font->ascent;
        descent = font->descent;
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