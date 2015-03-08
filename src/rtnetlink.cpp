#include "rtnetlink.h"

#include <linux/rtnetlink.h>

#include <netlink/route/link.h>

static void handle_event_obj(struct nl_object *obj, void *arg)
{
    Rtnetlink *inst = (Rtnetlink *) arg;
    int msgtype = nl_object_get_msgtype(obj);

    if (msgtype == RTM_NEWLINK) {
        inst->__handle_event_link((struct rtnl_link *)obj); 
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
    // TODO more groups
    
    event_loop.add_fd(this, nl_socket_get_fd(nl_event_sock));
}

void Rtnetlink::get_link_info(const char *ifname, LinkInfo &info)
{
    struct rtnl_link *link;

    if (rtnl_link_get_kernel(nl_info_sock, -1, ifname, &link) < 0) {
        fprintf(stderr, "unable to get link info for %s\n", ifname);
        abort();
    }

    info.iff_flags = rtnl_link_get_flags(link);

    rtnl_link_put(link);
}

void Rtnetlink::add_link_listener(LinkListener *listener)
{
    link_listeners.push_back(listener);
}

void Rtnetlink::__handle_event_link(struct rtnl_link *link) 
{
    for (LinkListener *listener : link_listeners) {
        listener->link_event(link);
    }
}

void Rtnetlink::descriptor_ready() noexcept {
    nl_recvmsgs_default(nl_event_sock);
}
