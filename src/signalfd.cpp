#include "signalfd.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <errno.h>

#include "utils.h"

SignalFd::SignalFd(EventLoop &event_loop, std::initializer_list<int> signals)
{
    int res;
    sigset_t mask;

    sigemptyset(&mask);

    for (int signo : signals) {
        sigaddset(&mask, signo);
    }

    res = sigprocmask(SIG_BLOCK, &mask, NULL);
    assert(res == 0);

    fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    check_fd(fd);

    event_loop.add_fd(this, fd);
}

SignalFd::~SignalFd()
{
    close(fd);
}

void SignalFd::descriptor_ready() noexcept
{
    while (true) {
        signalfd_siginfo siginfo;
        ssize_t res = read(fd, &siginfo, sizeof(siginfo));

        if (res < 0) {
            if (errno == EAGAIN) break;
            assert(false);
        }
        assert(res == sizeof(siginfo));
        for (auto *listener : listeners) {
            listener->received_signal(&siginfo);
        }
    }
}

void SignalFd::add_listener(SignalFdListener *listener)
{
    listeners.push_back(listener);
}
