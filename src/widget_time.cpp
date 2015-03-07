#include "widget_time.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "utils.h"

static const char * const FORMAT_STRING = 
    "{\"full_text\": \"%T\"}";

WidgetTime::WidgetTime(EventLoop &event_loop) {
    timerfd = create_timerfd(CLOCK_REALTIME, std::chrono::milliseconds(500));
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetTime::update_string() noexcept {
    struct timespec tp;
    struct tm result;

    if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
        perror("clock_gettime");
        abort();
    }

    if (localtime_r(&tp.tv_sec, &result) == nullptr) {
        perror("localtime_r");
        abort();
    }

    strftime(buffer, sizeof(buffer), FORMAT_STRING, &result);
}

const char * WidgetTime::get_string() const noexcept {
    return buffer;
}

void WidgetTime::descriptor_ready() noexcept {
    consume_timerfd(timerfd);
    update_string();
}
