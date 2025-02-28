module;
#include <memory>
import ui.event;

export module ui.event_listener;

export class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void handleEvent(const Event& event) = 0;
};