#include "widget_time.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "utils.h"

static const char * const FORMAT_STRING = 
    "{\"full_text\": \"%F %H:%M\"}";

WidgetTime::WidgetTime(EventLoop &event_loop) {
    timerfd = create_timerfd(
            CLOCK_REALTIME,
            std::chrono::seconds(60),
            std::chrono::seconds(60),
            true
    );
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetTime::update_string() noexcept {
    struct timespec tp;
    struct tm result, *result_p;
    size_t res;
    int err;

    err = clock_gettime(CLOCK_REALTIME, &tp);
    assert(err == 0);

    result_p = localtime_r(&tp.tv_sec, &result);
    assert(result_p != nullptr);

    res = strftime(buffer, sizeof(buffer), FORMAT_STRING, &result);
    assert(res != 0);
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
