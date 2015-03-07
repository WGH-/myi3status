#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <errno.h>
#include <unistd.h>

int create_timerfd_inner(int clockid, const struct timespec *interval)
{
    int res = timerfd_create(clockid, TFD_NONBLOCK | TFD_CLOEXEC);
    check_fd(res, "timerfd_create"); 

    struct itimerspec new_val;
    new_val.it_interval = *interval;
    new_val.it_value = *interval;

    if (timerfd_settime(res, 0, &new_val, nullptr) < 0) {
        perror("timerfd_settime");
        abort();
    }

    return res;
}

uint64_t consume_timerfd(int timerfd) noexcept
{
    uint64_t total_num_of_expirations = 0;
    for (;;) {
        uint64_t num_of_expirations;
        ssize_t res = read(timerfd, &num_of_expirations, sizeof(num_of_expirations));

        if (res < 0) {
            if (errno == EAGAIN) break;
            perror("WidgetTime, read(timerfd)");
            abort();
        }

        if (res != sizeof(num_of_expirations)) {
            fprintf(stderr, "Unexpected read from timerfd: %zd\n", res);
            abort();
        }
    
        assert(num_of_expirations > 0);

        total_num_of_expirations += num_of_expirations;
    }

    return total_num_of_expirations;
}

void check_fd(int fd, const char *perror_arg) noexcept {
    if (fd < 0) {
        perror(perror_arg);
        abort();
    }
}

unsigned long read_ulong_from_file(int fd) noexcept {
    char buffer[64]; // should be sufficient for everything
    char *endptr;
    unsigned long value;
    ssize_t res;

    res = read(fd, buffer, sizeof(buffer));

    if (res < 0) {
        perror("read");
        abort();
    }

    if (buffer[res-1] != '\n') {
        fprintf(stderr, "missing trailing newline\n");
        abort();
    }

    buffer[res-1] = '\0';

    value = strtoul(buffer, &endptr, 10);

    if (endptr != &buffer[res-1]) {
        fprintf(stderr, "strtoul conversion failed for '%s'\n", buffer);
        abort();
    }

    return value;
}
