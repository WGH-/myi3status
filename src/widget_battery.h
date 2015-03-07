#pragma once

#include "event_loop.h"

class WidgetBattery : public Widget {
    int timerfd;
    const char *battery_name;

    void update_string() noexcept;
    char buffer[32];
public:
    WidgetBattery(EventLoop &event_loop, const char *battery_name);
    virtual const char* get_string(void) const noexcept override;
    virtual void descriptor_ready() noexcept override;
};
