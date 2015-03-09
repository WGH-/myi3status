#pragma once

#include "event_loop.h"
#include "udev.h"

class WidgetAC : public Widget, UdevListener {
    const char *ac_name;
    const char *full_device_name; // as used by udev & co

    void update_string() noexcept;
    char buffer[64];
public:
    WidgetAC(EventLoop &event_loop, UdevMonitor &udev_monitor, const char *ac_name);
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void udev_event(struct udev_device *udev_device) noexcept override;
};
