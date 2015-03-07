#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define CACHED_PATH_DESCRIPTOR(ACCESSOR_NAME, PATH) \
    int ACCESSOR_NAME() { \
        static int fd = -1; \
        if (fd < 0) { \
            fd = open(PATH, O_RDONLY | O_DIRECTORY | O_PATH); \
        } \
        check_fd(fd); \
        return fd; \
    }

#include "cached_path_descriptors.h"
