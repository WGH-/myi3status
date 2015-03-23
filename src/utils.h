#pragma once

#include <chrono>

int create_timerfd_inner(int clockid, const struct timespec *interval, const struct timespec *value, bool absolute);

template<typename T>
int create_timerfd(int clockid, T interval, T value, bool absolute) {
    using namespace std::chrono;

    struct timespec interval2, value2;

    auto s = duration_cast<seconds>(interval);
    auto ns = duration_cast<nanoseconds>(interval - s);
    interval2.tv_sec = s.count();
    interval2.tv_nsec = ns.count();

    s = duration_cast<seconds>(value);
    ns = duration_cast<nanoseconds>(value - s);
    value2.tv_sec = s.count();
    value2.tv_nsec = ns.count();

    return create_timerfd_inner(clockid, &interval2, &value2, absolute);
}

template<typename T>
int create_timerfd(int clockid, T interval) {
    return create_timerfd(clockid, interval, interval, false);
}

template<>
inline int create_timerfd(int clockid, const struct timespec *interval, const struct timespec *value, bool absolute) {
    return create_timerfd_inner(clockid, interval, value, absolute);
}

uint64_t consume_timerfd(int fd) noexcept;

void check_fd(int fd, const char *perror_arg = nullptr) noexcept;

unsigned long read_ulong_from_file(int fd) noexcept;
