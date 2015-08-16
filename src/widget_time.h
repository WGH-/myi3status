#pragma once

#include "event_loop.h"

class WidgetTime : public Widget, Epollable {
    int timerfd;

    void update_string() noexcept;

    char buffer[64];

    class InotifyListener : public Epollable {
        int inotify_fd; // for monitoring timezone changes
        int etc_watch;
        WidgetTime &widget_time;

        void handle_event(const struct inotify_event *);

    public:
        InotifyListener(EventLoop &event_loop, WidgetTime &widget_time);
        virtual void descriptor_ready() noexcept override;
    };

    InotifyListener inotifyListener;
public:
    WidgetTime(EventLoop &event_loop);

    void localtime_updated() noexcept;

    virtual const char* get_string(bool force_update) noexcept override;
    virtual void descriptor_ready() noexcept override;
};
