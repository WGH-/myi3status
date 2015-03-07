#pragma once

#include <chrono>
#include <sys/timerfd.h>

int create_timerfd_inner(int clockid, const struct timespec *interval);

template<typename T>
int create_timerfd(int clockid, T duration) {
    struct timespec interval;

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

    interval.tv_sec = seconds.count();
    interval.tv_nsec = nanosec.count();

    return create_timerfd_inner(clockid, &interval);
}

template<>
inline int create_timerfd(int clockid, const struct timespec *interval) {
    return create_timerfd_inner(clockid, interval);
}

uint64_t consume_timerfd(int fd) noexcept;

void check_fd(int fd, const char *perror_arg = nullptr) noexcept;
