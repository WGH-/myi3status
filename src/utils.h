#pragma once

#include <sys/timerfd.h>

int create_timerfd(int clockid, const struct timespec *interval);
int create_timerfd_seconds(int clockid, time_t seconds);
int create_timerfd_nano(int clockid, long nsec);

static inline int create_timerfd_milli(int clockid, long millis) {
    return create_timerfd_nano(clockid, millis * 1000000L);
}
