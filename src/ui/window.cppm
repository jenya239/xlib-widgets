module;
#include <memory>  // Добавлено сюда

import ui.widget;
import ui.event;
import ui.event_listener;

export module ui.window;

export class Window : public Widget {
public:
    void handleEvent(const Event& event) override {
        Widget::handleEvent(event);
    }
};