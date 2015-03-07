#include "widget_ac.h"

#include <cstdio>
#include <cstdlib>

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

WidgetAC::WidgetAC(EventLoop &event_loop, const char *ac_name) {
    this->ac_name = ac_name;
    
    timerfd = create_timerfd(CLOCK_MONOTONIC, std::chrono::seconds(5));
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetAC::update_string() noexcept {
    int dirfd_ac = -1;

    buffer[0] = '\0';

    dirfd_ac = openat(CachedPathDescriptors::get_sysclasspowersupply(), ac_name, O_RDONLY | O_DIRECTORY | O_PATH);
    if (dirfd_ac < 0 && errno != ENOENT) {
        perror("openat(battery_name)");
        exit(1);
    }

    bool is_online = dirfd_ac && is_connected(dirfd_ac);

    snprintf(buffer, sizeof(buffer), "{\"full_text\": \"[" VOLTAGE_SYMBOL "]\", \"color\": \"%s\"}", is_online ? COLOR_ONLINE : COLOR_OFFLINE);

    close(dirfd_ac);
}

const char * WidgetAC::get_string() const noexcept {
    return buffer;
}

void WidgetAC::descriptor_ready() noexcept {
    consume_timerfd(timerfd);
    update_string();
}
