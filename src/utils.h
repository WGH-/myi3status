#pragma once

#include <chrono>

int create_timerfd_inner(int clockid, const struct timespec *interval, const struct timespec *value, bool absolute);

template<typename T>
struct timespec duration_to_timespec(T value)
{
    using namespace std::chrono;

    struct timespec result;

    auto s = duration_cast<seconds>(value);
    auto ns = duration_cast<nanoseconds>(value - s);

    result.tv_sec = s.count();
    result.tv_nsec = ns.count();

    return result;
}

template<typename T>
struct timespec timepoint_to_timespec(T value)
{
    return duration_to_timespec(value.time_since_epoch());
}

template<typename T>
int create_timerfd(int clockid, T interval, T value, bool absolute) {
    struct timespec interval2 = duration_to_timespec(interval),
                    value2 = duration_to_timespec(value);

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
