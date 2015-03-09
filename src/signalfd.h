#pragma once

#include <vector>

#include <signal.h>
#include <sys/signalfd.h>

class SignalFd;
class SignalFdListener {
public:
    virtual void received_signal(const struct signalfd_siginfo *) noexcept = 0;
};

#include "event_loop.h"

class SignalFd : public Epollable {
    int fd;
    std::vector<SignalFdListener*> listeners;
public:
    SignalFd(EventLoop &event_loop, std::initializer_list<int> signals);
    ~SignalFd();

    void descriptor_ready() noexcept override;

    void add_listener(SignalFdListener *);
private:
};
