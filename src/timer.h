#pragma once

#include "event_loop.h"
#include "utils.h"

#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

class TimerListener {
public:
    virtual void timer_ready() noexcept = 0;
};

class Timer : Epollable {
    int fd;
    boost::container::flat_set<TimerListener*> listeners;

    EventLoop &event_loop;
    const struct timespec interval;
public:
    Timer(EventLoop &event_loop, const struct timespec &interval);

    void register_listener(TimerListener *listener);
    void unregister_listener(TimerListener *listener);

    virtual void descriptor_ready() noexcept override;
};

class TimerManager {
    EventLoop &event_loop;
    boost::container::flat_map<struct timespec, Timer*> timer_cache;
public:
    TimerManager(EventLoop &event_loop);

    template<typename T>
    void register_monotonic_listener(TimerListener *listener, T interval)
    {
        register_monotonic_listener(listener, duration_to_timespec(interval));
    }

    void register_monotonic_listener(TimerListener *listener, const struct timespec &interval);

    void unregister_monotonic_listener(TimerListener *listener);
};
