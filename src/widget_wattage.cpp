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

WidgetWattage::WidgetWattage(EventLoop &event_loop, std::initializer_list<const char*> batteries) {
    for (const char *battery_name : batteries) {
        this->batteries.push_back(battery_name);
    }

    timerfd = create_timerfd(CLOCK_MONOTONIC, std::chrono::seconds(15));
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetWattage::update_string() noexcept {
    unsigned long power_now = 0;
    for (const std::string &battery_name : batteries) {
        power_now += get_wattage(battery_name.c_str());
    }

    snprintf(buffer, sizeof(buffer), "{\"full_text\": \"%.1lf W\"}", power_now / 1000.0 / 1000.0);
}

const char * WidgetWattage::get_string(bool force_update) noexcept {
    if (force_update) {
        update_string();
    }
    return buffer;
}

void WidgetWattage::descriptor_ready() noexcept {
    consume_timerfd(timerfd);
    update_string();
}
