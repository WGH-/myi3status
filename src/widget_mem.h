#pragma once

#include "event_loop.h"
#include "timer.h"

#include <cstdio>

class WidgetMem : public Widget, TimerListener {
    void update_string() noexcept;

    char buffer[64];
public:
    WidgetMem(TimerManager &timer_manager, unsigned long poll_interval_ms);
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void timer_ready() noexcept override;
};
