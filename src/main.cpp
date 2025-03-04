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
import services.render_service;
import ui.application;
import ui.application_window;
import ui.button;
import ui.event;
import ui.widget;

int main() {
    try {
        // Создаем и инициализируем приложение
        auto app = std::make_shared<Application>();

        // Создаем главное окно
        if (!app->createMainWindow("My Application", 800, 600)) {
            return 1;
        }

        auto mainWindow = app->getMainWindow();
        auto logger = app->getLogger();

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

        button->onMouseDown = [&logger]() {
            logger->debug("Button state changed to PRESSED");
        };

        button->onMouseUp = [&logger]() {
            logger->debug("Button state changed to HOVER (after press)");
        };

        // Добавляем кнопку в окно
        mainWindow->addChild(button);

        // Настраиваем обработку событий
        app->setupEventHandling();

        // Выполняем первоначальную отрисовку
        app->initialRender();

        // Запускаем цикл обработки событий
        app->run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}