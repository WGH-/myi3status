#include "utils.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int create_timerfd(int clockid, const struct timespec *interval)
{
    int res = timerfd_create(clockid, TFD_NONBLOCK | TFD_CLOEXEC);
    if (res < 0) {
        perror("timerfd_create");
        exit(1);
    }

    struct itimerspec new_val;
    new_val.it_interval = *interval;
    new_val.it_value = *interval;

    if (timerfd_settime(res, 0, &new_val, nullptr) < 0) {
        perror("timerfd_settime");
        exit(1);
    }

    return res;
}

int create_timerfd_seconds(int clockid, time_t seconds)
{
    struct timespec interval;
    interval.tv_sec = seconds;
    interval.tv_nsec = 0;
    return create_timerfd(clockid, &interval);
}

int create_timerfd_nano(int clockid, long nanoseconds)
{
    struct timespec interval;
    interval.tv_sec = 0;
    interval.tv_nsec = nanoseconds;
    return create_timerfd(clockid, &interval);
}
