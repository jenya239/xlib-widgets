#include <memory>
#include <iostream>

import ui.widget;
import ui.event;

class Button : public Widget {
public:
    void handleEvent(const Event& event) override {
        if (event.type == "click") {
            std::cout << "Button clicked!\n";
        }
    }
};

int main() {
    auto root = std::make_shared<Widget>();
    auto button = std::make_shared<Button>();

    root->addChild(button);
    root->handleEvent(Event("click"));

    return 0;
}