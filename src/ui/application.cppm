module;
#include <memory>
#include <string>
#include <X11/Xlib.h>

export module ui.application;

import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import services.render_service;  // Make sure this is properly imported
import ui.application_window;
import ui.button;
import ui.event;
import ui.widget;

export class Application {
private:
    std::shared_ptr<XDisplayService> displayService;
    std::shared_ptr<EventLoopService> eventLoop;
    std::shared_ptr<LoggerService> logger;
    std::shared_ptr<ApplicationWindow> mainWindow;

public:
    Application() {
        // Initialize services
        displayService = std::make_shared<XDisplayService>();
        // XDisplayService constructor already initializes the display
        // No need to call initialize() as it doesn't exist

        logger = std::make_shared<LoggerService>();

        // Pass references instead of shared_ptr objects
        eventLoop = std::make_shared<EventLoopService>(*displayService, *logger);
    }

    ~Application() {
        // Clean up resources
        // XDisplayService destructor already handles cleanup
        // No need to call cleanup() as it doesn't exist
    }

    bool createMainWindow(const std::string& title, int width, int height) {
        mainWindow = std::make_shared<ApplicationWindow>(title, width, height);
        return mainWindow->initialize(displayService->getDisplay());
    }

    std::shared_ptr<ApplicationWindow> getMainWindow() const {
        return mainWindow;
    }

    std::shared_ptr<LoggerService> getLogger() const {
        return logger;
    }

    void setupEventHandling() {
        if (!mainWindow) return;

        Display* display = displayService->getDisplay();
        Window xWindow = mainWindow->getNativeWindow();

        // Register event handler for the main window
        eventLoop->registerEventHandler(xWindow, [this](const Event& event) {
            if (mainWindow) {
                mainWindow->handleEvent(event);
            }
        });
    }

    void initialRender() {
        if (!mainWindow) return;

        Display* display = displayService->getDisplay();
        Window xWindow = mainWindow->getNativeWindow();
        GC gc = XCreateGC(display, xWindow, 0, nullptr);

        // Render all widgets
        RenderService::repaintDirtyWidgets(display, mainWindow->getNativeWindow(), gc, mainWindow.get());

        XFreeGC(display, gc);
        XFlush(display);
    }

    void run() {
        if (!eventLoop || !mainWindow) return;

        Display* display = displayService->getDisplay();
        Window xWindow = mainWindow->getNativeWindow();

        // Set up periodic redraw
        eventLoop->setRedrawCallback([this]() {
            if (!mainWindow) return;

            Display* display = displayService->getDisplay();
            Window xWindow = mainWindow->getNativeWindow();
            GC gc = XCreateGC(display, xWindow, 0, nullptr);

            // Redraw all widgets that need updating
            RenderService::repaintDirtyWidgets(display, xWindow, gc, mainWindow.get());

            XFreeGC(display, gc);
            XFlush(display);
        });

        // Initial render
        GC gc = XCreateGC(display, xWindow, 0, nullptr);
        RenderService::repaintDirtyWidgets(display, xWindow, gc, mainWindow.get());
        XFreeGC(display, gc);
        XFlush(display);

        // Run the event loop
        eventLoop->run();
    }
};