#include "widget_battery.h"

#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "cached_descriptors.h"

static float get_battery_level(int battery_dirfd) {
    int energynow_fd, energyfull_fd;

    energynow_fd = openat(battery_dirfd, "energy_now", O_RDONLY);
    check_fd(energynow_fd, "openat(..., \"energy_now\")");

    energyfull_fd = openat(battery_dirfd, "energy_full", O_RDONLY);
    check_fd(energyfull_fd, "openat(..., \"energy_full\")");

    unsigned long energy_now = read_ulong_from_file(energynow_fd);
    unsigned long energy_full = read_ulong_from_file(energyfull_fd);

    close(energynow_fd);
    close(energyfull_fd);

    return float(energy_now) / energy_full;
}

WidgetBattery::WidgetBattery(TimerManager &timer_manager, const char *battery_name, unsigned long poll_interval_ms) {
    this->battery_name = battery_name;

    timer_manager.register_monotonic_listener(this, std::chrono::milliseconds(poll_interval_ms));

    update_string();
}

void WidgetBattery::update_string() noexcept {
    int dirfd_battery = -1;

    buffer[0] = '\0';

    // we can't cache battery directory
    // because it may come and go as battery is attached/detached
    dirfd_battery = openat(CachedDescriptors::get_sysclasspowersupply(), battery_name, O_RDONLY | O_DIRECTORY | O_PATH);
    // note that battery might be not present
    if (dirfd_battery < 0 && errno != ENOENT) {
        perror("openat(battery_name)");
        exit(1);
    }

    if (dirfd_battery >= 0) {
        float battery_level = get_battery_level(dirfd_battery);
        const char *color = "#FFFFFF";
        if (battery_level < 0.105) {
            color = "#FFFF00";
        }
        if (battery_level < 0.055) {
            color = "#FF0000";
        }
        snprintf(buffer, sizeof(buffer), "{\"color\":\"%s\",\"full_text\": \"%s: %3.0f%%\"}", color, battery_name, battery_level * 100);
    }

    close(dirfd_battery);
}

const char * WidgetBattery::get_string(bool force_update) noexcept {
    if (force_update) {
        update_string();
    }
    return buffer;
}

void WidgetBattery::timer_ready() noexcept {
    update_string();
}
