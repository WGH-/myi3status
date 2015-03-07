#pragma once

#include "event_loop.h"

class WidgetAC : public Widget {
    const char *ac_name;
    int timerfd;

    void update_string() noexcept;
    char buffer[64];
public:
    WidgetAC(EventLoop &event_loop, const char *ac_name);
    virtual const char* get_string(void) const noexcept override;
    virtual void descriptor_ready() noexcept override;
};
