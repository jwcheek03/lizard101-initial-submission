#include "EventHandler.h" // Make sure this exists and is included
#include "Event.h"

class InputHandler : public EventHandler {
public:
    void onEvent(const Event& e) override {
        if (e.type == EventType::Input) {
            // handle input event
        }
    }
};