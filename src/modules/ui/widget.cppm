module;
#include <vector>
#include <memory>  // Добавлено сюда

import ui.event;
import ui.event_listener;

export module ui.widget;

export class Widget : public EventListener {
public:
    virtual void handleEvent(const Event& event) override {
        for (auto& child : children) {
            child->handleEvent(event);
        }
    }

    void addChild(std::shared_ptr<Widget> child) {
        children.push_back(std::move(child));
    }

private:
    std::vector<std::shared_ptr<Widget>> children;
};