#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

int create_timerfd_inner(int clockid, const struct timespec *interval, const struct timespec *value, bool absolute)
{
    int err;
    int fd = timerfd_create(clockid, TFD_NONBLOCK | TFD_CLOEXEC);
    check_fd(fd, "timerfd_create"); 

    struct itimerspec new_val;
    new_val.it_interval = *interval;
    new_val.it_value = *value;

    int settime_flags = 0;
    if (absolute) {
        settime_flags |= TFD_TIMER_ABSTIME;
    }

    err = timerfd_settime(fd, settime_flags, &new_val, nullptr);
    assert(err == 0);

    return fd;
}

uint64_t consume_timerfd(int timerfd) noexcept
{
    uint64_t num_of_expirations;
    ssize_t res = read(timerfd, &num_of_expirations, sizeof(num_of_expirations));

    if (res < 0) {
        if (errno == EAGAIN) return 0;;
        assert(false);
    }

    assert(res == sizeof(num_of_expirations));
    assert(num_of_expirations > 0);

    return num_of_expirations;
}

void check_fd(int fd, const char *perror_arg) noexcept {
    assert(fd >= 0);
}

unsigned long read_ulong_from_file(int fd) noexcept {
    char buffer[64]; // should be sufficient for everything
    char *endptr;
    unsigned long value;
    ssize_t res;

    res = read(fd, buffer, sizeof(buffer));
    assert(res >= 0);

    assert(buffer[res-1] == '\n');

    buffer[res-1] = '\0';

    value = strtoul(buffer, &endptr, 10);

    assert(endptr == &buffer[res-1] && "strtoul failed");

    return value;
}
