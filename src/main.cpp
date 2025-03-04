// main.cpp
#include <X11/Xlib.h>
#include <memory>
#include <iostream>
#include <functional>
#include <cstring>
#include <typeinfo>

// Для использования C++20 модулей в обычном .cpp файле нужно использовать import
import services.xdisplay_service;
import services.event_loop_service;
import services.logger_service;
import ui.application_window;
import ui.button;
import ui.event;
import ui.widget;

// Объявления функций перед их использованием
void repaintDirtyWidgets(Display* display, Window window, GC gc, Widget* rootWidget);
void repaintWidgetIfDirty(Widget* widget, Display* display, Window window, GC gc);

int main() {
    // Создаем сервис для работы с X11 Display
    auto displayService = std::make_shared<XDisplayService>();
    auto logger = std::make_shared<LoggerService>();

    // Получаем Display
    Display* display = displayService->getDisplay();
    if (!display) {
        std::cerr << "Не удалось получить X11 Display" << std::endl;
        return 1;
    }

    // Создаем EventLoopService с необходимыми зависимостями
    auto eventLoop = std::make_shared<EventLoopService>(displayService, logger);

    // Создаем окно приложения
    auto windowWidget = std::make_shared<ApplicationWindow>("My Application", 800, 600);
    if (!windowWidget->initialize(display)) {
        std::cerr << "Не удалось инициализировать окно приложения" << std::endl;
        return 1;
    }

    // Получаем X11 Window из ApplicationWindow
    Window xWindow = windowWidget->getNativeWindow();

    // Создаем кнопку
    auto button = std::make_shared<Button>("Click Me!");
    button->setPosition(100, 100);
    button->setSize(200, 50);

    // Настраиваем обработчики событий для кнопки
    button->onClick = [&logger]() {
        logger->info("Button clicked!");
    };

    button->onMouseEnter = [&logger, button]() {
        logger->info("Mouse entered button: " + button->getLabel());
        logger->debug("Button state changed to HOVER");
    };

    button->onMouseLeave = [&logger, button]() {
        logger->info("Mouse left button: " + button->getLabel());
        logger->debug("Button state changed to NORMAL");
    };

    // Добавляем отладку для изменения состояния кнопки при нажатии
    button->onMouseDown = [&logger]() {
        logger->debug("Button state changed to PRESSED");
    };

    button->onMouseUp = [&logger]() {
        logger->debug("Button state changed to HOVER (after press)");
    };

    // Добавляем кнопку в окно
    windowWidget->addChild(button);

    // Создаем графический контекст
    GC gc = XCreateGC(display, xWindow, 0, nullptr);
    if (!gc) {
        std::cerr << "Не удалось создать графический контекст" << std::endl;
        return 1;
    }

    logger->info("Window created successfully: " + std::to_string(xWindow));

    // Помечаем все виджеты как "грязные" для первоначальной отрисовки
    windowWidget->markDirty();
    button->markDirty();

    // Выполняем первоначальную отрисовку всех виджетов
    windowWidget->render(display, xWindow);
    repaintDirtyWidgets(display, xWindow, gc, windowWidget.get());

    // Отправляем событие Expose для первоначальной отрисовки
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = Expose;
    xev.xexpose.window = xWindow;
    XSendEvent(display, xWindow, False, ExposureMask, &xev);
    XFlush(display);

    // Регистрируем обработчик событий для окна
    eventLoop->registerEventHandler(xWindow, [&](const Event& event) {
        // Убираем отладочную информацию для событий движения мыши

        // Обрабатываем событие в виджетах
        windowWidget->handleEvent(event);

        // Обрабатываем различные типы событий
        if (event.type == EventType::PaintEvent) {
            windowWidget->render(display, xWindow);
        }
        else if (event.type == EventType::Unknown) {
            // Для обработки закрытия окна
            // eventLoop->stop();
        }

        // Перерисовываем "грязные" виджеты
        repaintDirtyWidgets(display, xWindow, gc, windowWidget.get());
        XFlush(display); // Добавляем явный вызов XFlush для обновления экрана
    });

    // Запускаем цикл обработки событий в основном потоке
    logger->info("Starting event loop");
    eventLoop->start();  // Блокирующий вызов, который завершится при вызове eventLoop->stop()
    logger->info("Event loop stopped");

    // Освобождаем ресурсы
    XFreeGC(display, gc);

    return 0;
}

// Функция перерисовки "грязных" виджетов
void repaintDirtyWidgets(Display* display, Window window, GC gc, Widget* rootWidget) {
    if (!rootWidget) return;

    // Перерисовываем текущий виджет, если он "грязный"
    repaintWidgetIfDirty(rootWidget, display, window, gc);

    // Рекурсивно перерисовываем дочерние виджеты
    for (Widget* child : rootWidget->getChildren()) {
        repaintDirtyWidgets(display, window, gc, child);
    }
}

// Только функция repaintWidgetIfDirty
void repaintWidgetIfDirty(Widget* widget, Display* display, Window window, GC gc) {
    if (widget && widget->needsRepaint()) {
        std::cout << "Repainting widget: " << typeid(*widget).name() << std::endl;
        widget->paint(display, window, gc);
        widget->clearDirty();

        // Добавляем явный вызов XFlush для обновления экрана
        XFlush(display);
    }
}