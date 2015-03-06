#include "event_loop.h"

#define MAX_EVENTS 8

#include <array>

#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <sys/epoll.h>

EventLoop::EventLoop() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        exit(1);
    }
}

void EventLoop::run() noexcept {
    std::array<struct epoll_event, MAX_EVENTS> events;

    puts("{\"version\":1}");
    puts("["); // start of infinite list

    for (;;) {
        int nevents = epoll_wait(epoll_fd, events.begin(), events.size(), -1);

        for (int i = 0; i < nevents; i++) {
            Widget *widget = (Widget *) events[i].data.ptr;
            widget->descriptor_ready();
        }

        if (nevents > 0) {
            print_stuff();
        }
    }

    puts("]"); // end of infinite list
}

void EventLoop::print_stuff() noexcept {
    puts("[");
    bool need_comma = false;
    for (Widget *widget : widgets) {
        if (need_comma) {
            putchar(',');
        } else {
            need_comma = true;
        }
        puts(widget->get_string());
    }
    puts("],");
    fflush(stdout);
}

void EventLoop::add_widget(Widget *widget) noexcept {
    widgets.push_back(widget);
}

void EventLoop::add_fd(Widget *widget, int fd) noexcept {
    struct epoll_event event;

    event.data.ptr = widget;
    event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_ctl(..., EPOLL_CTL_ADD, ...)");
        exit(1);
    }
}
