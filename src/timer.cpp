#include "timer.h"

Timer::Timer(EventLoop &event_loop, const struct timespec &interval)
{
    fd = create_timerfd(CLOCK_MONOTONIC, &interval);
    event_loop.add_fd(this, fd);
}

void Timer::descriptor_ready() noexcept
{
    consume_timerfd(fd);

    for (TimerListener *listener : listeners) {
        listener->timer_ready();
    }
}

void Timer::register_listener(TimerListener *listener)
{
    listeners.push_back(listener);
}

TimerManager::TimerManager(EventLoop &event_loop)
    : event_loop(event_loop)
{

}

bool operator<(const struct timespec &a, const struct timespec &b)
{
    return std::tie(a.tv_sec, a.tv_nsec) < std::tie(b.tv_sec, b.tv_nsec);
}

void TimerManager::register_monotonic_listener(TimerListener *listener, const struct timespec &interval)
{
    Timer *t;

    auto it = timer_cache.find(interval);

    if (it == timer_cache.end()) {
        t = new Timer(event_loop, interval);
        timer_cache[interval] = t;
    } else {
        t = it->second;
    }

    t->register_listener(listener);
}
