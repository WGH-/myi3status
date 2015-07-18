#include "widget_mem.h"
#include "utils.h"

#include <cstddef>
#include <cassert>
#include <cstring>
#include <unistd.h>

#include "cached_descriptors.h"

static const char * const FORMAT_STRING = 
    "{"
        "\"full_text\":\"%lu/%lu M\","
        "\"color\": \"%s\""
    "}";

static const char * const COLOR_NORMAL = "#FFFFFF";
static const char * const COLOR_75 = "#FFFF00";
static const char * const COLOR_90 = "#FF0000";

struct MemInfo {
    unsigned long mem_total;
    unsigned long mem_available;

    static struct {
        const char *field_name;
        size_t offset;
    } meminfo_offsets[];
public:
    void fill_meminfo(int fd);
};

decltype(MemInfo::meminfo_offsets) MemInfo::meminfo_offsets = {
    {"MemTotal", offsetof(struct MemInfo, mem_total)},
    {"MemAvailable", offsetof(struct MemInfo, mem_available)},
    {nullptr, 0} // sentinel
};

void MemInfo::fill_meminfo(int fd)
{
    char buffer[2048];
    ssize_t n;

    lseek(fd, 0, SEEK_SET);
    n = read(fd, buffer, sizeof(buffer));
    assert(n > 0);
    assert(n < sizeof(buffer));
    buffer[n] = '\0';

    const char *p = buffer;
    while (true) {
        if (*p == '\0') break;

        const char *colonp = strchr(p, ':');
        if (colonp == nullptr) break;
        
        for (auto meminfo_offset = meminfo_offsets; meminfo_offset->field_name != nullptr; meminfo_offset++) {
            if (strncmp(meminfo_offset->field_name, p, colonp - p) == 0) {
                unsigned long *ulong = (unsigned long *)(((char*)this) + meminfo_offset->offset);
                int ret = sscanf(colonp+1, "%lu kB", ulong);
                assert(ret == 1);
                break;
            }
        }
        
        p = strchr(p, '\n'); // points to \n
        assert(p != nullptr);
        p++; // point to next line OR null byte (if it was the last line)
    }
}

WidgetMem::WidgetMem(TimerManager &timer_manager, unsigned long poll_interval_ms)
{
    timer_manager.register_monotonic_listener(this, std::chrono::milliseconds(poll_interval_ms));

    update_string();
}

void WidgetMem::update_string()
{
    MemInfo meminfo;
    const char *color = COLOR_NORMAL;

    meminfo.fill_meminfo(CachedDescriptors::get_procmeminfo());

    unsigned long total = meminfo.mem_total;
    unsigned long usage = meminfo.mem_total - meminfo.mem_available;

    unsigned long percent = usage * 100 /total;

    if (percent >= 90) {
        color = COLOR_90;
    } else if (percent >= 75) {
        color = COLOR_75;
    }

    snprintf(buffer, sizeof(buffer), FORMAT_STRING, 
        usage / 1024, 
        total / 1024,
        color
    );
}

const char *WidgetMem::get_string(bool force_update) noexcept
{
    if (force_update) {
        update_string();
    }
    return buffer;
}

void WidgetMem::timer_ready() noexcept {
    update_string();
}
