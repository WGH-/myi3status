#pragma once

#include <netlink/netlink.h>

#include "event_loop.h"
#include "nl80211.h"

class Widget_nl80211 : public Widget, Nl80211Listener {
    char string[128];
    const char *ifname;
    
    Nl80211 &nl80211;
    Nl80211::InterfaceInfo info;

    void get_updated_info() noexcept;
    void update_string() noexcept;
public:
    Widget_nl80211(Nl80211 &nl80211, const char *ifname);

    virtual const char* get_string(void) const noexcept override;
    virtual void nl80211event(struct nl_msg *msg) noexcept override;
};
