#pragma once

#include <pulse/pulseaudio.h>

#include "event_loop.h"
#include "timer_oneshot.h"

#include <thread>
#include <atomic>

class Widget_Pulse : public Widget, Epollable, OneshotTimerListener {
    void update_string() noexcept;
    char string[64];
    const char *sink_name;

    std::atomic<pa_volume_t> volume;

    int fd;
    void notify_eventfd();
    void update_volume(pa_context *c);

    std::thread worker_thread;
    void worker_thread_function();

    OneshotTimerManager &oneshot_timer_manager;

    bool hide_widget;

    TimerHandle *last_timer;

public:
    void context_state_callback(pa_context *c);
    void context_subscribe_callback(
        pa_context *c,
        pa_subscription_event_type_t t,
        uint32_t idx);
    void get_sink_volume_callback(
        pa_context *c,
        const pa_sink_info *i,
        int is_last);

    Widget_Pulse(EventLoop &event_loop,
            OneshotTimerManager &oneshot_timer_manager,
            const char *sink_name);
    ~Widget_Pulse();

    virtual const char* get_string(bool force_update) noexcept override;
    virtual void descriptor_ready() noexcept override;
    virtual void oneshot_timer_ready() noexcept override;
};
