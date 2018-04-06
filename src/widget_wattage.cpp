#include "widget_wattage.h"

#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "cached_descriptors.h"

static unsigned long get_wattage(const char *battery_name)
{
    int dirfd_battery = -1;
    int fd_powernow = -1;
    unsigned long result = 0;

    dirfd_battery = openat(CachedDescriptors::get_sysclasspowersupply(), battery_name, O_RDONLY | O_DIRECTORY | O_PATH);

    if (dirfd_battery < 0 && errno != ENOENT) {
        perror("openat(battery_name)");
        exit(1);
    }

    if (dirfd_battery >= 0) {
        fd_powernow = openat(dirfd_battery, "power_now", O_RDONLY);
        result = read_ulong_from_file(fd_powernow);
    }

    close(fd_powernow);
    close(dirfd_battery);

    return result;
}

WidgetWattage::WidgetWattage(TimerManager &timer_manager, std::initializer_list<const char*> batteries, unsigned poll_interval_ms) {
    for (const char *battery_name : batteries) {
        this->batteries.push_back(battery_name);
    }

    timer_manager.register_monotonic_listener(this, std::chrono::milliseconds(poll_interval_ms));

    update_string();
}

void WidgetWattage::update_string() noexcept {
    unsigned long power_now = 0;
    for (const std::string &battery_name : batteries) {
        power_now += get_wattage(battery_name.c_str());
    }

    if (power_now == 0) {
        buffer[0] = '\0';
    } else {
        snprintf(buffer, sizeof(buffer), "{\"full_text\": \"%.1lf W\"}", power_now / 1000.0 / 1000.0);
    }
}

const char * WidgetWattage::get_string(bool force_update) noexcept {
    if (force_update) {
        update_string();
    }
    return buffer;
}

void WidgetWattage::timer_ready() noexcept {
    update_string();
}
