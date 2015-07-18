#pragma once

#include <netlink/netlink.h>

#include "event_loop.h"
#include "timer.h"
#include "nl80211.h"
#include "rtnetlink.h"

class Widget_nl80211 : public Widget, Nl80211Listener, Rtnetlink::LinkListener, TimerListener {
    char string[128];
    const char *ifname;
    unsigned long signal_poll_interval_ms;
    
    Nl80211 &nl80211;
    Nl80211::InterfaceInfo info;

    Rtnetlink &rtnetlink;
    Rtnetlink::LinkInfo link_info;
    TimerManager &timer_manager;

    void update_string() noexcept;
public:
    Widget_nl80211(TimerManager &timer_manager, Nl80211 &nl80211, Rtnetlink &rtnetlink, const char *ifname, unsigned long signal_poll_interval_ms);

    virtual const char* get_string(bool force_update) noexcept override;
    virtual void nl80211event(struct nl_msg *msg) noexcept override;
    virtual void link_event(struct rtnl_link *link) noexcept override;
    virtual void timer_ready() noexcept override;
};
