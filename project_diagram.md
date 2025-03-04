# Диаграмма проекта

```mermaid
classDiagram
    class IoC {
        +return instance
        +IoC instance
        +return instance
        +hash_code()
        +registerSingleton(std::shared_ptr<T> instance)
        +resolve()
        +typeid(T)
        +find(typeId)
        +find(typeId)
        +getInstance()
    }
    class EventLoopService {
        +XEvent xEvent
        +Type type
        +logger(logger)
        +move(handler)
        +move(callback)
        +run()
        +getDisplay()
        +now()
        +while(running && display)
        +XNextEvent(display, &xEvent)
        +convertXEvent(xEvent)
        +find(xEvent.xany.window)
        +second(event)
        +now()
        +redrawCallback()
        +stop()
        +convertXEvent(const XEvent& xEvent)
        +switch(xEvent.type)
        +Event(type, x, y)
    }
    class LogLevel {
    }
    class LoggerService {
        +setLogLevel(LogLevel level)
        +debug(const std::string& message)
        +if(currentLevel <= LogLevel::DEBUG)
        +info(const std::string& message)
        +if(currentLevel <= LogLevel::INFO)
        +warning(const std::string& message)
        +if(currentLevel <= LogLevel::WARNING)
        +error(const std::string& message)
        +if(currentLevel <= LogLevel::ERROR)
    }
    class RenderService {
        +return dirtyWidgets
        +repaintDirtyWidgets(Display* display, Window window, GC gc, Widget* rootWidget)
        +render(display, window)
        +paint(display, window, gc)
        +findDirtyWidgets(Widget* rootWidget)
        +collectDirtyWidgets(rootWidget, dirtyWidgets)
        +collectDirtyWidgets(Widget* widget, std::vector<Widget*>& dirtyWidgets)
        +push_back(widget)
        +collectDirtyWidgets(child, dirtyWidgets)
    }
    class XDisplayService {
        +return display
        +return window
        +XDisplayService()
        +XOpenDisplay(nullptr)
        +if(!display)
        +runtime_error("Failed to open X display")
        +XDisplayService()
        +if(display)
        +XCloseDisplay(display)
        +XDisplayService(const XDisplayService&)
        +XDisplayService(XDisplayService&& other)
        +if(this != &other)
        +if(display)
        +XCloseDisplay(display)
        +getDisplay()
        +createWindow(int x, int y, unsigned int width, unsigned int height)
        +DefaultScreen(display)
        +RootWindow(display, screen)
        +BlackPixel(display, screen)
        +WhitePixel(display, screen)
        +XCreateSimpleWindow(
            display, root, x, y, width, height, 1, black, white
        )
        +mapWindow(Window window)
        +XMapWindow(display, window)
        +flush()
        +XFlush(display)
        +createGC(Window window)
        +XCreateGC(display, window, 0, nullptr)
        +drawRectangle(Window window, GC gc, int x, int y, unsigned int width, unsigned int height)
        +XDrawRectangle(display, window, gc, x, y, width, height)
        +fillRectangle(Window window, GC gc, int x, int y, unsigned int width, unsigned int height)
        +XFillRectangle(display, window, gc, x, y, width, height)
        +setForeground(GC gc, unsigned long color)
        +XSetForeground(display, gc, color)
        +drawText(Window window, GC gc, int x, int y, const std::string& text)
    }
    class Application {
        +shared_ptr<XDisplayService> displayService
        +shared_ptr<EventLoopService> eventLoop
        +shared_ptr<LoggerService> logger
        +shared_ptr<ApplicationWindow> mainWindow
        +return mainWindow
        +return logger
        +Application()
        +Application()
        +createMainWindow(const std::string& title, int width, int height)
        +getMainWindow()
        +getLogger()
        +setupEventHandling()
        +getDisplay()
        +getNativeWindow()
        +registerEventHandler(xWindow, [this](const Event& event)
        +if(mainWindow)
        +handleEvent(event)
        +initialRender()
        +getDisplay()
        +getNativeWindow()
        +XCreateGC(display, xWindow, 0, nullptr)
        +XFreeGC(display, gc)
        +XFlush(display)
        +run()
        +getDisplay()
        +getNativeWindow()
        +setRedrawCallback([this]()
        +getDisplay()
        +getNativeWindow()
        +XCreateGC(display, xWindow, 0, nullptr)
        +XFreeGC(display, gc)
        +XFlush(display)
        +XCreateGC(display, xWindow, 0, nullptr)
        +XFreeGC(display, gc)
        +XFlush(display)
        +run()
    }
    class ApplicationWindow {
        +return false
        +return false
        +return true
        +return display
        +return xWindow
        +return xWindow
        +ApplicationWindow()
        +height(height)
        +initialize(Display* display)
        +DefaultScreen(display)
        +BlackPixel(display, screen)
        +WhitePixel(display, screen)
        +XSelectInput(display, xWindow,
            ExposureMask | ButtonPressMask | ButtonReleaseMask |
            KeyPressMask | StructureNotifyMask | PointerMotionMask)
        +XMapWindow(display, xWindow)
        +XFlush(display)
        +setSize(width, height)
        +setNativeWindow(Display* display, Window xWindow)
        +getDisplay()
        +getNativeWindow()
        +getWindow()
        +handleEvent(const Event& event)
        +handleEvent(event)
        +repaint()
        +repaint()
        +render(display, xWindow)
        +XCreateGC(display, xWindow, 0, nullptr)
        +paint(display, xWindow, gc)
        +XFreeGC(display, gc)
        +XFlush(display)
    }
    class ButtonState {
    }
    class Button {
    }
    class Event {
    }
    class Type {
        +Type type
        +int x
        +int y
        +int button
        +int keycode
        +XEvent nativeEvent
        +return type
        +return x
        +return y
        +return button
        +return keycode
        +return nativeEvent
        +keycode(keycode)
        +nativeEvent(xEvent)
        +switch(xEvent.type)
        +getType()
        +getX()
        +getY()
        +getButton()
        +getKeycode()
        +getNativeEvent()
    }
    class EventListener {
        +EventListener()
        +handleEvent(const Event& event)
    }
    class RenderBuffer {
        +Pixmap pixmap
        +GC gc
        +int width
        +int height
        +return pixmap
        +return gc
        +return valid
        +return false
        +return false
        +return true
        +return width
        +return height
        +height(height)
        +if(!display)
        +invalid_argument("Display pointer cannot be null")
        +if(width == 0 || height == 0)
        +invalid_argument("Width and height must be greater than zero")
        +if(!pixmap)
        +runtime_error("Failed to create pixmap")
        +XCreateGC(display, pixmap, 0, nullptr)
        +if(!gc)
        +XFreePixmap(display, pixmap)
        +runtime_error("Failed to create graphics context")
        +XFillRectangle(display, pixmap, gc, 0, 0, width, height)
        +RenderBuffer()
        +if(valid)
        +XFreeGC(display, gc)
        +XFreePixmap(display, pixmap)
        +RenderBuffer(const RenderBuffer&)
        +valid(other.valid)
        +if(this != &other)
        +if(valid)
        +XFreeGC(display, gc)
        +XFreePixmap(display, pixmap)
        +getPixmap()
        +getGC()
        +clear(unsigned long color)
        +XSetForeground(display, gc, color)
        +XFillRectangle(display, pixmap, gc, 0, 0, width, height)
        +copyTo(Drawable dest, int srcX, int srcY, int destX, int destY,
                unsigned int width, unsigned int height)
        +if(!valid)
        +if(width > 0 && height > 0)
        +XCreateGC(display, dest, 0, nullptr)
        +if(tempGC)
        +XCopyArea(display, pixmap, dest, tempGC, srcX, srcY, width, height, destX, destY)
        +XFreeGC(display, tempGC)
        +XCopyArea(display, pixmap, dest, gc, srcX, srcY, width, height, destX, destY)
        +XFlush(display)
        +copyToWindow(::Window window, int x, int y)
        +copyTo(window, 0, 0, x, y, width, height)
        +isValid()
        +resize(::Window window, unsigned int newWidth, unsigned int newHeight)
        +min(width, newWidth)
        +min(height, newHeight)
        +XCopyArea(display, pixmap, newPixmap, gc, 0, 0, copyWidth, copyHeight, 0, 0)
        +XFreePixmap(display, pixmap)
        +if(newWidth > copyWidth)
        +XFillRectangle(display, pixmap, gc, copyWidth, 0, newWidth - copyWidth, copyHeight)
        +if(newHeight > copyHeight)
        +XFillRectangle(display, pixmap, gc, 0, copyHeight, newWidth, newHeight - copyHeight)
        +getWidth()
        +getHeight()
    }
    class Widget {
        +return x
        +return y
        +return width
        +return height
        +return isDirty
        +return rawPointers
        +return currentWindow
        +unique_ptr<RenderBuffer> buffer
        +handleEvent(const Event& event)
        +for(auto& child : children)
        +handleEvent(event)
        +setPosition(int x, int y)
        +if(this->x != x || this->y != y)
        +markDirty()
        +setSize(unsigned int width, unsigned int height)
        +if(this->width != width || this->height != height)
        +if(buffer)
        +reset()
        +markDirty()
        +getX()
        +getY()
        +getWidth()
        +getHeight()
        +render(Display* display, Window window)
        +setWindow(window)
        +if(isDirty)
        +ensureBuffer(display, window)
        +paintToBuffer(display)
        +clearDirty()
        +for(auto& child : children)
        +render(display, window)
        +ensureBuffer(Display* display, ::Window window)
        +if(!buffer)
        +paint(Display* display, Window window, GC gc)
        +ensureBuffer(display, window)
        +copyTo(window, 0, 0, x, y, width, height)
        +for(auto& child : children)
        +paint(display, window, gc)
        +paintToBuffer(Display* display)
        +getBuffer()
        +get()
        +markDirty()
        +needsRepaint()
        +clearDirty()
        +setParent(Widget* parent)
        +addChild(std::shared_ptr<Widget> child)
        +setParent(this)
        +getChildren()
        +for(const auto& child : children)
        +setWindow(Window window)
        +getWindow()
    }
    IoC --> IoC
    Application --> XDisplayService
    Application --> EventLoopService
    Application --> LoggerService
    ApplicationWindow --|> Widget
    Button --|> Widget
    Type --> Type
    Widget --|> EventListener
    Widget --> RenderBuffer
    services_event_loop_service ..> services_xdisplay_service : imports
    services_event_loop_service ..> services_logger_service : imports
    services_event_loop_service ..> ui_event : imports
    services_render_service ..> ui_widget : imports
    services_render_service ..> ui_render_buffer : imports
    ui_application ..> services_xdisplay_service : imports
    ui_application ..> services_event_loop_service : imports
    ui_application ..> services_logger_service : imports
    ui_application ..> services_render_service : imports
    ui_application ..> ui_application_window : imports
    ui_application ..> ui_button : imports
    ui_application ..> ui_event : imports
    ui_application ..> ui_widget : imports
    ui_application_window ..> ui_widget : imports
    ui_application_window ..> ui_event : imports
    ui_application_window ..> ui_event_listener : imports
    ui_application_window ..> ui_render_buffer : imports
    ui_button ..> ui_widget : imports
    ui_button ..> ui_event : imports
    ui_button ..> ui_event_listener : imports
    ui_button ..> ui_render_buffer : imports
    ui_event_listener ..> ui_event : imports
    ui_widget ..> ui_event : imports
    ui_widget ..> ui_event_listener : imports
    ui_widget ..> ui_render_buffer : imports
```

## Легенда

- `-->` : Ассоциация (один класс использует другой)
- `--|>` : Наследование
- `..>` : Зависимость (импорт модуля)
