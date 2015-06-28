#pragma once

#include <vector>
#include <string>

#include "event_loop.h"

class WidgetWattage : public Widget, Epollable {
    int timerfd;
    std::vector<std::string> batteries;

    void update_string() noexcept;
    char buffer[32];
public:
    WidgetWattage(EventLoop &event_loop, std::initializer_list<const char*> batteries, unsigned poll_interval_ms);
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void descriptor_ready() noexcept override;
};
