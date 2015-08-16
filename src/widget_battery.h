#pragma once

#include "event_loop.h"
#include "timer.h"

class WidgetBattery : public Widget, TimerListener {
    int timerfd;
    const char *battery_name;

    void update_string() noexcept;
    char buffer[64];
public:
    WidgetBattery(TimerManager &timer_manager, const char *battery_name, unsigned long poll_interval_ms);
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void timer_ready() noexcept override;
};
