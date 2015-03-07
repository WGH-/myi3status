#include "widget_battery.h"

#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "cached_path_descriptors.h"

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

WidgetBattery::WidgetBattery(EventLoop &event_loop, const char *battery_name) {
    this->battery_name = battery_name;

    timerfd = create_timerfd(CLOCK_MONOTONIC, std::chrono::seconds(5));
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetBattery::update_string() noexcept {
    int dirfd_battery = -1;

    buffer[0] = '\0';

    // we can't cache battery directory
    // because it may come and go as battery is attached/detached
    dirfd_battery = openat(CachedPathDescriptors::get_sysclasspowersupply(), battery_name, O_RDONLY | O_DIRECTORY | O_PATH);
    // note that battery might be not present
    if (dirfd_battery < 0 && errno != ENOENT) {
        perror("openat(battery_name)");
        exit(1);
    }

    if (dirfd_battery >= 0) {
        snprintf(buffer, sizeof(buffer), "{\"full_text\": \"%s: %2.0f%%\"}", battery_name, get_battery_level(dirfd_battery) * 100);
    }

    close(dirfd_battery);
}

const char * WidgetBattery::get_string() const noexcept {
    return buffer;
}

void WidgetBattery::descriptor_ready() noexcept {
    consume_timerfd(timerfd);
    update_string();
}