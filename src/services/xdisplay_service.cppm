// src/services/xdisplay_service.cppm
module;
#include <memory>
#include <X11/Xlib.h>
#include <stdexcept>
#include <string>

export module services.xdisplay_service;

export class XDisplayService {
private:
    Display* display = nullptr;

public:
    XDisplayService() {
        display = XOpenDisplay(nullptr);
        if (!display) {
            throw std::runtime_error("Failed to open X display");
        }
    }

    ~XDisplayService() {
        if (display) {
            XCloseDisplay(display);
            display = nullptr;
        }
    }

    // Prevent copying
    XDisplayService(const XDisplayService&) = delete;
    XDisplayService& operator=(const XDisplayService&) = delete;

    // Allow moving
    XDisplayService(XDisplayService&& other) noexcept {
        display = other.display;
        other.display = nullptr;
    }

    XDisplayService& operator=(XDisplayService&& other) noexcept {
        if (this != &other) {
            if (display) {
                XCloseDisplay(display);
            }
            display = other.display;
            other.display = nullptr;
        }
        return *this;
    }

    Display* getDisplay() const {
        return display;
    }

    // Create a simple window
    Window createWindow(int x, int y, unsigned int width, unsigned int height) {
        int screen = DefaultScreen(display);
        Window root = RootWindow(display, screen);
        unsigned long black = BlackPixel(display, screen);
        unsigned long white = WhitePixel(display, screen);

        Window window = XCreateSimpleWindow(
            display, root, x, y, width, height, 1, black, white
        );

        return window;
    }

    // Map window to screen
    void mapWindow(Window window) {
        XMapWindow(display, window);
    }

    // Flush display
    void flush() {
        XFlush(display);
    }

    // Create a graphics context
    GC createGC(Window window) {
        return XCreateGC(display, window, 0, nullptr);
    }

    // Draw rectangle
    void drawRectangle(Window window, GC gc, int x, int y, unsigned int width, unsigned int height) {
        XDrawRectangle(display, window, gc, x, y, width, height);
    }

    // Fill rectangle
    void fillRectangle(Window window, GC gc, int x, int y, unsigned int width, unsigned int height) {
        XFillRectangle(display, window, gc, x, y, width, height);
    }

    // Set foreground color
    void setForeground(GC gc, unsigned long color) {
        XSetForeground(display, gc, color);
    }

    // Draw text
    void drawText(Window window, GC gc, int x, int y, const std::string& text) {
        XDrawString(display, window, gc, x, y, text.c_str(), text.length());
    }
};
