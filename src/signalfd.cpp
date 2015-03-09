#include "signalfd.h"

#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <errno.h>

#include "utils.h"

SignalFd::SignalFd(EventLoop &event_loop, std::initializer_list<int> signals)
{
    sigset_t mask;
    sigemptyset(&mask);
    for (int signo : signals) {
        sigaddset(&mask, signo);
    }

    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
        abort();
    }

    fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    check_fd(fd);

    event_loop.add_fd(this, fd);
}

void SignalFd::descriptor_ready() noexcept
{
    while (true) {
        signalfd_siginfo siginfo;
        ssize_t res = read(fd, &siginfo, sizeof(siginfo));
        if (res < 0) {
            if (errno == EAGAIN) break;
            perror("read(signalfd)");
            abort();
        }
        if (res != sizeof(siginfo)) {
            fprintf(stderr, "partial read siginfo\n");
            abort();
        }
        for (auto *listener : listeners) {
            listener->received_signal(&siginfo);
        }
    }
}

void SignalFd::add_listener(SignalFdListener *listener)
{
    listeners.push_back(listener);
}
