#pragma once
#include "Event.h"

struct EventHandler {
    virtual ~EventHandler() = default;
    virtual void onEvent(const Event& e) = 0;
};
