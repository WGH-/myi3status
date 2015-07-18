#pragma once

#include <vector>
#include <string>

#include "event_loop.h"
#include "timer.h"

class WidgetWattage : public Widget, TimerListener {
    std::vector<std::string> batteries;

    void update_string() noexcept;
    char buffer[32];
public:
    WidgetWattage(TimerManager &timer_manager, std::initializer_list<const char*> batteries, unsigned poll_interval_ms);
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void timer_ready() noexcept override;
};
