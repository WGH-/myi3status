#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define CACHED_DESCRIPTOR(ACCESSOR_NAME, PATH, FLAGS) \
    int ACCESSOR_NAME() { \
        static int fd = -1; \
        if (fd < 0) { \
            fd = open(PATH, FLAGS); \
            check_fd(fd); \
        } \
        return fd; \
    }

#include "cached_descriptors.h"
