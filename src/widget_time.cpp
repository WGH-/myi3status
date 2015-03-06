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
    timerfd = create_timerfd_milli(CLOCK_REALTIME, 500);
    event_loop.add_fd(this, timerfd);

    update_string();
}

void WidgetTime::update_string() noexcept {
    struct timespec tp;
    struct tm result;

    if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
        perror("clock_gettime");
        exit(1);
    }

    if (localtime_r(&tp.tv_sec, &result) == nullptr) {
        perror("localtime_r");
        exit(1);
    }

    strftime(buffer, sizeof(buffer), FORMAT_STRING, &result);
}

const char * WidgetTime::get_string() const noexcept {
    return buffer;
}

void WidgetTime::descriptor_ready() noexcept {
    for (;;) {
        uint64_t num_of_expirations;
        ssize_t res = read(timerfd, &num_of_expirations, sizeof(num_of_expirations));

        if (res < 0) {
            if (errno == EAGAIN) break;
            perror("WidgetTime, read(timerfd)");
            exit(1);
        }

        if (res != sizeof(num_of_expirations)) {
            fprintf(stderr, "Unexpected read from timerfd: %zd\n", res);
            exit(1);
        }

        update_string();
    }
}
