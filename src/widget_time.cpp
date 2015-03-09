#include "widget_time.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "utils.h"

static const char * const FORMAT_STRING = 
    "{\"full_text\": \"%F %H:%M\"}";

WidgetTime::WidgetTime(EventLoop &event_loop) {
    timerfd = create_timerfd(CLOCK_REALTIME, std::chrono::seconds(10));
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetTime::update_string() noexcept {
    struct timespec tp;
    struct tm result;
    size_t res;

    if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
        perror("clock_gettime");
        abort();
    }

    if (localtime_r(&tp.tv_sec, &result) == nullptr) {
        perror("localtime_r");
        abort();
    }

    res = strftime(buffer, sizeof(buffer), FORMAT_STRING, &result);
    if (res == 0) {
        fprintf(stderr, "strftime failed\n");
        abort();
    }
}

const char * WidgetTime::get_string(bool force_update) noexcept {
    if (force_update) {
        update_string();
    }
    return buffer;
}

void WidgetTime::descriptor_ready() noexcept {
    consume_timerfd(timerfd);
    update_string();
}
