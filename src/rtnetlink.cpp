#include "rtnetlink.h"

#include <linux/rtnetlink.h>

#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/rtnl.h>

static void handle_event_obj(struct nl_object *obj, void *arg)
{
    Rtnetlink *inst = (Rtnetlink *) arg;
    int msgtype = nl_object_get_msgtype(obj);

    if (msgtype == RTM_NEWLINK || msgtype == RTM_DELLINK) {
        inst->__handle_event_link((struct rtnl_link *)obj); 
    }
    if (msgtype == RTM_NEWADDR || msgtype == RTM_DELADDR) {
        inst->__handle_event_addr((struct rtnl_addr *)obj);
    }
}

int handle_event(struct nl_msg *msg, void *arg) 
{
    Rtnetlink *inst = (Rtnetlink *) arg;

    if (nl_msg_parse(msg, &handle_event_obj, arg) < 0) {
        // ignore for now
    }

    return NL_OK;
}

Rtnetlink::Rtnetlink(EventLoop &event_loop) {
    create_info_sock(); 
    create_event_sock(event_loop);
}

void Rtnetlink::create_info_sock() {
    nl_info_sock = nl_socket_alloc();

    if (!nl_info_sock) {
        fprintf(stderr, "couldn't allocate netlink socket\n");
        abort();
    }
    
    nl_socket_set_buffer_size(nl_info_sock, 8192, 8192);

    if (nl_connect(nl_info_sock, NETLINK_ROUTE)) {
        fprintf(stderr, "couldn't connect to netlink socket\n");
        abort();
    } 
}

void Rtnetlink::create_event_sock(EventLoop &event_loop) {
    nl_event_sock = nl_socket_alloc();

    if (!nl_event_sock) {
        fprintf(stderr, "couldn't allocate netlink socket\n");
        abort();
    }
    
    nl_socket_set_buffer_size(nl_event_sock, 8192, 8192);

    if (nl_connect(nl_event_sock, NETLINK_ROUTE)) {
        fprintf(stderr, "couldn't connect to netlink socket\n");
        abort();
    } 
    
    nl_socket_disable_seq_check(nl_event_sock);
    nl_socket_set_nonblocking(nl_event_sock);

    nl_socket_modify_cb(nl_event_sock, NL_CB_VALID, NL_CB_CUSTOM, handle_event, (void *) this);

    nl_socket_add_membership(nl_event_sock, RTNLGRP_LINK);
    nl_socket_add_membership(nl_event_sock, RTNLGRP_IPV4_IFADDR);
    nl_socket_add_membership(nl_event_sock, RTNLGRP_IPV6_IFADDR);
    // TODO more groups
    
    event_loop.add_fd(this, nl_socket_get_fd(nl_event_sock));
}

void Rtnetlink::force_addr_update()
{
    // note that we use event socket here
    // TODO switch to blocking, and then to nonblocking again
    nl_rtgen_request(nl_event_sock, RTM_GETADDR, AF_UNSPEC, NLM_F_DUMP);
}

void Rtnetlink::get_link_info(const char *ifname, LinkInfo &info, struct rtnl_link *link)
{
    bool borrowed_reference;

    if (link == nullptr) {
        if (rtnl_link_get_kernel(nl_info_sock, -1, ifname, &link) < 0) {
            fprintf(stderr, "unable to get link info for %s\n", ifname);
            abort();
        }
        borrowed_reference = false;
    } else {
        borrowed_reference = true;
    }

    info.iff_flags = rtnl_link_get_flags(link);

    if (!borrowed_reference) {
        rtnl_link_put(link);
    }
}

void Rtnetlink::fill_addr_info(AddrInfo &info, struct rtnl_addr *addr)
{
    int msgtype = nl_object_get_msgtype((struct nl_object *) addr);
    struct nl_addr *local_addr = rtnl_addr_get_local(addr);

    // TODO multiple IP support

    if (nl_addr_get_family(local_addr) != AF_INET) {
        // TODO support IPv6
        return;
    } 
    struct in_addr *ipv4 = (struct in_addr *) nl_addr_get_binary_addr(local_addr);

    if (msgtype == RTM_DELADDR) {
        if (memcmp(&info.addr, ipv4, sizeof(info.addr)) == 0) {
            info.invalid = true;
        }
    }
    if (msgtype == RTM_NEWADDR) {
        if (!info.invalid && (ipv4->s_addr & 0x0000fea9) == 0x0000fea9) {
            // ignore zeroconf ipv4 addresses
            return;
        }

        memcpy(&info.addr, ipv4, sizeof(info.addr));
        info.invalid = false;
    }
}

void Rtnetlink::add_link_listener(LinkListener *listener)
{
    link_listeners.push_back(listener);
}

void Rtnetlink::add_addr_listener(AddrListener *listener)
{
    addr_listeners.push_back(listener);
}

void Rtnetlink::__handle_event_link(struct rtnl_link *link) 
{
    for (auto *listener : link_listeners) {
        listener->link_event(link);
    }
}

void Rtnetlink::__handle_event_addr(struct rtnl_addr *addr) 
{
    for (auto *listener : addr_listeners) {
        listener->addr_event(addr);
    }
}

void Rtnetlink::descriptor_ready() noexcept {
    nl_recvmsgs_default(nl_event_sock);
}
