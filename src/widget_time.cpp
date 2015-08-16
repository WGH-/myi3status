#include "widget_time.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <sys/inotify.h>

#include "utils.h"

static const char * const FORMAT_STRING =
    "{\"full_text\": \"%F %H:%M\"}";

WidgetTime::WidgetTime(EventLoop &event_loop) :
    inotifyListener(event_loop, *this)
{
    timerfd = create_timerfd(
            CLOCK_REALTIME,
            std::chrono::seconds(60),
            std::chrono::seconds(60),
            true
    );
    event_loop.add_fd(this, timerfd);

    tzset();
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

void WidgetTime::localtime_updated() noexcept {
    tzset();
    update_string();
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

WidgetTime::InotifyListener::InotifyListener(EventLoop &event_loop, WidgetTime &widget_time)
    : widget_time(widget_time)
{
    int res;

    inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    assert(inotify_fd >= 0);

    res = inotify_add_watch(inotify_fd, "/etc", IN_MOVED_TO | IN_CREATE);
    assert(res >= 0);
    etc_watch = res;

    event_loop.add_fd(this, inotify_fd);
}

void WidgetTime::InotifyListener::descriptor_ready() noexcept {
    char buf[4096]
        __attribute__((aligned(__alignof__(struct inotify_event))));

    static_assert(sizeof(buf) >= sizeof(struct inotify_event) + NAME_MAX + 1, "");

    while (1) {
        ssize_t len;
        char *ptr;
        const struct inotify_event *event;

        len = read(inotify_fd, buf, sizeof buf);
        if (len == -1 && errno != EAGAIN) {
            assert(false);
        }

        if (len <= 0) break;

        for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
        {
            event = (const struct inotify_event *) ptr;
            handle_event(event);
        }
    }
}

void WidgetTime::InotifyListener::handle_event(const struct inotify_event *event)
{
    if (event->wd == etc_watch) {
        if (event->mask & (IN_MOVED_TO | IN_CREATE)) {
            if (strcmp(event->name, "localtime") == 0) {
                widget_time.localtime_updated();
            }
        }
    }
}
