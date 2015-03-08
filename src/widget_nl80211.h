#pragma once

#include <netlink/netlink.h>

#include "event_loop.h"
#include "nl80211.h"
#include "rtnetlink.h"

class Widget_nl80211 : public Widget, Nl80211Listener, Rtnetlink::LinkListener {
    char string[128];
    const char *ifname;
    
    Nl80211 &nl80211;
    Nl80211::InterfaceInfo info;

    Rtnetlink &rtnetlink;
    Rtnetlink::LinkInfo link_info;

    void update_string(struct rtnl_link *link = nullptr) noexcept;
public:
    Widget_nl80211(Nl80211 &nl80211, Rtnetlink &rtnetlink, const char *ifname);

    virtual const char* get_string(void) const noexcept override;
    virtual void nl80211event(struct nl_msg *msg) noexcept override;
    virtual void link_event(struct rtnl_link *link) noexcept override;
};
