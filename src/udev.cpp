#include "udev.h"

#include <cstdio>
#include <cstdlib>

#define UDEV_MONITOR_RECV_BUFFER (128 * 1024 * 1024)

UdevMonitor::UdevMonitor(EventLoop &event_loop) 
{
    struct udev *udev;
    
    udev = udev_new();
    if (udev == nullptr) {
        fprintf(stderr, "couldn't create udev object\n");
        abort();
    }

    udev_monitor = udev_monitor_new_from_netlink(udev, "udev"); 
    
    if (udev_monitor == nullptr) {
        fprintf(stderr, "couldn't create udev_monitor object\n");
        abort();
    }

    udev_monitor_set_receive_buffer_size(udev_monitor, UDEV_MONITOR_RECV_BUFFER);
     
    udev_fd = udev_monitor_get_fd(udev_monitor);

    if (udev_monitor_enable_receiving(udev_monitor) < 0) {
        fprintf(stderr, "couldn't subscribe to udev events\n");
        abort();
    }

    event_loop.add_fd(this, udev_fd);
}

void UdevMonitor::add_listener(UdevListener *listener)
{
    listeners.push_back(listener); 
}
    
void UdevMonitor::descriptor_ready() noexcept {
    for (;;) {
        struct udev_device *udev_device;
        udev_device = udev_monitor_receive_device(udev_monitor);
        if (udev_device == nullptr) break;

        handle_device(udev_device);

        udev_device_unref(udev_device);
    }
}

void UdevMonitor::handle_device(struct udev_device *udev_device) noexcept {
    for (UdevListener *listener : listeners) {
        listener->udev_event(udev_device);
    }
}
