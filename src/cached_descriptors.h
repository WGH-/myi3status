#pragma once

namespace CachedDescriptors {

#ifndef CACHED_DESCRIPTOR
#define CACHED_DESCRIPTOR(ACCESSOR_NAME, PATH, FLAGS) \
    int ACCESSOR_NAME();
#endif

    CACHED_DESCRIPTOR(get_sysclasspowersupply, "/sys/class/power_supply", O_RDONLY | O_DIRECTORY | O_PATH)
    CACHED_DESCRIPTOR(get_procmeminfo, "/proc/meminfo", O_RDONLY)

#undef CACHED_DESCRIPTOR
}
