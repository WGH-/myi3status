#pragma once

#include <vector>

#include <libudev.h>

#include "event_loop.h"

class UdevListener {
public:
    virtual void udev_event(struct udev_device *udev_device) noexcept = 0;
};

class UdevMonitor : public Epollable {
    int udev_fd;
    struct udev_monitor *udev_monitor;
    std::vector<UdevListener*> listeners;

    void handle_device(struct udev_device *udev_device) noexcept;
public:
    UdevMonitor(EventLoop &event_loop);

    virtual void descriptor_ready() noexcept override;
    void add_listener(UdevListener *listener);
};
