#pragma once

#include <vector>
#include <string>

class Epollable {
public:
    virtual void descriptor_ready() noexcept = 0;
};

class EventLoop;

#include "signalfd.h"

class Widget {
public:
    virtual const char* get_string(bool force_update = false) noexcept = 0;
};

class EventLoop : SignalFdListener {
    int epoll_fd;
    std::vector<Widget*> widgets;
    
    bool force_next_update;
    void print_stuff() noexcept;

    std::string front_buffer, back_buffer;
public:
    EventLoop();

    void add_fd(Epollable *widget, int fd) noexcept;
    void remove_fd(int fd) noexcept;

    void run() noexcept;

    void add_widget(Widget *widget) noexcept;

    virtual void received_signal(const struct signalfd_siginfo *) noexcept override;
};


