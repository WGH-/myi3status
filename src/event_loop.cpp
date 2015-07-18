#include "event_loop.h"

#define MAX_EVENTS 8

#include <array>

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <sys/epoll.h>

#include "utils.h"

EventLoop::EventLoop() {
    epoll_fd = epoll_create1(0);
    check_fd(epoll_fd, "epoll_create1");
    
    force_next_update = false;
}

void EventLoop::run() noexcept {
    std::array<struct epoll_event, MAX_EVENTS> events;

    fputs("{\"version\":1}\n", stdout);
    fputs("[\n", stdout); // start of infinite list

    // always print the first "update" immediately
    print_stuff();

    SignalFd signalFdUsr1(*this, {SIGUSR1});
    signalFdUsr1.add_listener(this);

    for (;;) {
        int nevents = epoll_wait(epoll_fd, events.begin(), events.size(), -1);

        for (int i = 0; i < nevents; i++) {
            Epollable *epollable = (Epollable *) events[i].data.ptr;
            epollable->descriptor_ready();
        }

        if (nevents > 0) {
            print_stuff();
        }
    }

    fputs("]\n", stdout); // end of infinite list
}

void EventLoop::print_stuff() noexcept {
    front_buffer.resize(0);

    front_buffer += '[';

    bool need_comma = false;

    for (Widget *widget : widgets) {
        const char *s = widget->get_string(force_next_update);

        if (!s || !s[0]) continue;

        if (need_comma) {
            front_buffer += ',';
        } else {
            need_comma = true;
        }

        front_buffer += s;
    }
    force_next_update = false;

    if (front_buffer != back_buffer) {
        fputs(front_buffer.c_str(), stdout);
        fputs("],\n", stdout);
        fflush(stdout);

        std::swap(front_buffer, back_buffer);
    }
}
    
void EventLoop::received_signal(const struct signalfd_siginfo *siginfo) noexcept {
    if (siginfo->ssi_signo == SIGUSR1) {
        force_next_update = true;
    }
}

void EventLoop::add_widget(Widget *widget) noexcept {
    widgets.push_back(widget);
}

void EventLoop::add_fd(Epollable *epollable, int fd) noexcept {
    struct epoll_event event;
    int res;

    event.data.ptr = epollable;
    event.events = EPOLLIN;

    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    assert(res == 0);
}

void EventLoop::remove_fd(int fd) noexcept {
    int res;

    res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    assert(res == 0);
}
