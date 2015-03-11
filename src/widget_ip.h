#pragma once

#include "event_loop.h"
#include "rtnetlink.h"

class Widget_IP : public Widget, Rtnetlink::AddrListener {
    char string[128];
    const char *ifname;
    const char *color;

    Rtnetlink &rtnetlink;
    Rtnetlink::AddrInfo addr_info;

    void update_data(struct rtnl_addr *addr) noexcept;
    void update_string() noexcept;
public:
    Widget_IP(Rtnetlink &rtnetlink, const char *ifname, const char *color = nullptr);
    
    virtual const char* get_string(bool force_update) noexcept override;
    virtual void addr_event(struct rtnl_addr *addr) noexcept override;
};
