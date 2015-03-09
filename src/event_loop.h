#pragma once

#include <vector>

class Epollable {
public:
    virtual void descriptor_ready() noexcept = 0;
};

class Widget {
public:
    virtual const char* get_string(bool force_update = false) noexcept = 0;
};

class EventLoop {
    int epoll_fd;
    std::vector<Widget*> widgets;

    void print_stuff() noexcept;
public:
    EventLoop();

    void add_fd(Epollable *widget, int fd) noexcept;
    void remove_fd(int fd) noexcept;

    void run() noexcept;

    void add_widget(Widget *widget) noexcept;
};


