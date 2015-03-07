#pragma once

namespace CachedPathDescriptors {

#ifndef CACHED_PATH_DESCRIPTOR
#define CACHED_PATH_DESCRIPTOR(ACCESSOR_NAME, PATH) \
    int ACCESSOR_NAME();
#endif

    CACHED_PATH_DESCRIPTOR(get_sysclasspowersupply, "/sys/class/power_supply")

#undef CACHED_PATH_DESCRIPTOR
}
