#pragma once

#include <vector>

#include <netlink/netlink.h>

#include "event_loop.h"

class Rtnetlink : public Epollable {
    // netlink socket for request/response
    struct nl_sock *nl_info_sock;
    //struct nl_cb *nl_info_cb, *nl_info_s_cb;
    // netlink socket for monitoring
    struct nl_sock *nl_event_sock;
    
    // constructor helpers
    void create_info_sock();
    void create_event_sock(EventLoop &event_loop);
public:
    Rtnetlink(EventLoop &event_loop);

    virtual void descriptor_ready() noexcept override;

    // get_link_info
    struct LinkInfo {
        unsigned iff_flags; 
    };
    void get_link_info(const char *ifname, LinkInfo &info);
    
    // link status updates
    class LinkListener {
    public:
        virtual void link_event(struct rtnl_link *link) noexcept = 0;
    };
    void add_link_listener(LinkListener *listener);

    // "private" callbacks
    void __handle_event_link(struct rtnl_link *link);
private:
    std::vector<LinkListener*> link_listeners;
};
