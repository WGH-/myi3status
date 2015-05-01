#include "widget_ac.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "cached_path_descriptors.h"

const char * const COLOR_OFFLINE = "#404040";
const char * const COLOR_ONLINE = "#F0F000";

//#define VOLTAGE_SYMBOL "\xe2\x86\xaf" // DOWNWARDS ZIGZAG ARROW
#define VOLTAGE_SYMBOL "\xe2\x9a\xa1" // HIGH VOLTAGE SIGN

static bool is_connected(int ac_dirfd) {
    int online_fd;

    online_fd = openat(ac_dirfd, "online", O_RDONLY);
    check_fd(online_fd, "openat(..., \"online\")");

    unsigned long online = read_ulong_from_file(online_fd);

    close(online_fd);

    return online != 0;
}

/*
 * This function essentially reads a link in
 * /sys/class/power_supply, and preprocesses it to the format
 * that udev uses
 */
static const char * get_device_name(const char *symlink_name) {
    char path[256];
    const char *device_name;
    ssize_t res;

    res = readlinkat(CachedPathDescriptors::get_sysclasspowersupply(), symlink_name, path, sizeof(path));

    assert(res >= 0);

    assert(size_t(res) < sizeof(path) - 1);

    path[res] = '\0'; // readlink doesn't put a null terminator

    device_name = strstr(path, "/devices/");
    assert(device_name != nullptr);

    return strdup(device_name);
}

WidgetAC::WidgetAC(EventLoop &event_loop, UdevMonitor &udev_monitor, const char *ac_name) {
    this->ac_name = ac_name;
    
    full_device_name = get_device_name(ac_name);

    udev_monitor.add_listener(this);
    
    update_string();
}

void WidgetAC::update_string() noexcept {
    int dirfd_ac = -1;

    buffer[0] = '\0';

    dirfd_ac = openat(CachedPathDescriptors::get_sysclasspowersupply(), ac_name, O_RDONLY | O_DIRECTORY | O_PATH);
    assert(dirfd_ac >= 0 || errno == ENOENT);

    bool is_online = dirfd_ac >= 0 && is_connected(dirfd_ac);

    snprintf(buffer, sizeof(buffer), "{\"full_text\": \"[" VOLTAGE_SYMBOL "]\", \"color\": \"%s\"}", is_online ? COLOR_ONLINE : COLOR_OFFLINE);

    close(dirfd_ac);
}

const char * WidgetAC::get_string(bool force_update) noexcept {
    // this one doesn't do polling, so no need to forcibly update
    return buffer;
}

void WidgetAC::udev_event(struct udev_device *udev_device) noexcept {
    const char *event_device_name = udev_device_get_devpath(udev_device);

    if (strcmp(event_device_name, full_device_name) == 0) {
        update_string();
    }
}
