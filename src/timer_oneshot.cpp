#include "timer_oneshot.h"

#include <sys/timerfd.h>
#include <unistd.h>

#define CLOCK_ID CLOCK_MONOTONIC

OneshotTimerManager::OneshotTimerManager(EventLoop &event_loop)
{
    fd = timerfd_create(CLOCK_ID, TFD_NONBLOCK | TFD_CLOEXEC);
    check_fd(fd, "oneshot timerfd");

    event_loop.add_fd(this, fd);
}

OneshotTimerManager::~OneshotTimerManager()
{
    close(fd);
}

void OneshotTimerManager::descriptor_ready()
{
    consume_timerfd(fd);
    check_queue();
}

void OneshotTimerManager::schedule_after(clock_t::duration dt, OneshotTimerListener *listener)
{
    auto expiration = clock_t::now() + dt;

    queue.emplace(expiration, listener);

    check_queue();

}

void OneshotTimerManager::check_queue()
{
    auto now = clock_t::now();

    while (!queue.empty() && now >= queue.top().first) {
        auto expired = queue.top();
        expired.second->oneshot_timer_ready();
        queue.pop();
    }

    struct itimerspec new_val;
    new_val.it_interval = duration_to_timespec(std::chrono::nanoseconds::zero());
    if (!queue.empty()) {
        new_val.it_value = timepoint_to_timespec(queue.top().first);
    } else {
        new_val.it_value = duration_to_timespec(std::chrono::nanoseconds::zero());
    }
    // TODO check error
    timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_val, nullptr);
}
