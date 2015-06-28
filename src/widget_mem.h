#pragma once

#include "event_loop.h"

#include <cstdio>

class WidgetMem : public Widget, Epollable {
    int timerfd;
    void update_string() noexcept;

    char buffer[64];
public:
    WidgetMem(EventLoop &event_loop, unsigned long poll_interval_ms);
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void descriptor_ready() noexcept override;
};
