#pragma once

#include <vector>

// forward declaration
class Widget;

class EventLoop {
    int epoll_fd;
    std::vector<Widget*> widgets;

    void print_stuff() noexcept;
public:
    EventLoop();

    void add_fd(Widget *widget, int fd) noexcept;
    void remove_fd(int fd) noexcept;

    void run() noexcept;

    void add_widget(Widget *widget) noexcept;
};

class Widget {
public:
    virtual const char* get_string(void) const noexcept = 0;
    virtual void descriptor_ready() noexcept = 0;
};
