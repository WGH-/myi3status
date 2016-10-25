#pragma once

#include "event_loop.h"
#include "utils.h"

#include <queue>
#include <memory>

/*
 * XXX assumes std::chrono::steady_clock is backed by CLOCK_MONOTONIC
 */

class OneshotTimerListener {
public:
    virtual void oneshot_timer_ready() noexcept = 0;
};

class OneshotTimerManager;

class TimerHandle {
    OneshotTimerListener *listener;
    bool is_cancelled;

    TimerHandle(OneshotTimerListener *listener)
        : listener(listener), is_cancelled(false)
    {}

    friend class OneshotTimerManager;
public:
    void cancel() {
        is_cancelled = true;
    }
};

class OneshotTimerManager : Epollable {
public:
    typedef std::chrono::steady_clock clock_t;

    OneshotTimerManager(EventLoop &event_loop);
    ~OneshotTimerManager();

    virtual void descriptor_ready() noexcept override;

    TimerHandle* schedule_after(struct timespec *it_value, OneshotTimerListener *);

    TimerHandle* schedule_after(clock_t::duration dt, OneshotTimerListener *listener);

protected:
    int fd;

    typedef std::pair<
        std::chrono::steady_clock::time_point,
        std::unique_ptr<TimerHandle>
    > queue_entry_t;

    std::priority_queue<
        queue_entry_t,
        std::vector<queue_entry_t>,
        std::greater<queue_entry_t>
    > queue;

    clock_t::time_point current_expiration;

    void check_queue();
};
