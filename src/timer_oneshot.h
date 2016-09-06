#pragma once

#include "event_loop.h"
#include "utils.h"

#include <queue>

/*
 * XXX assumes std::chrono::steady_clock is backed by CLOCK_MONOTONIC
 */

class OneshotTimerListener {
public:
    virtual void oneshot_timer_ready() noexcept = 0;
};

class OneshotTimerManager : Epollable {
public:
    typedef std::chrono::steady_clock clock_t;

    OneshotTimerManager(EventLoop &event_loop);
    ~OneshotTimerManager();

    virtual void descriptor_ready() noexcept override;

    void schedule_after(struct timespec *it_value, OneshotTimerListener *);

    void schedule_after(clock_t::duration dt, OneshotTimerListener *listener);

protected:
    int fd;

    typedef std::pair<
        std::chrono::steady_clock::time_point,
        OneshotTimerListener *
    > queue_entry_t;

    std::priority_queue<
        queue_entry_t,
        std::vector<queue_entry_t>,
        std::greater<queue_entry_t>
    > queue;

    clock_t::time_point current_expiration;

    void check_queue();
};
