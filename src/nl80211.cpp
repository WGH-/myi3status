#include "nl80211.h"

#include <cstdio>
#include <cstdlib>
#include <cctype>

#include <net/if.h>

#include <linux/nl80211.h>

#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "ext/genl.h"

static int no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

// utility functions
static void add_to_multicast_group(struct nl_sock *nl_sock, const char *group_name) {
    int mcid, ret;

    mcid = nl_get_multicast_id(nl_sock, "nl80211", group_name);

    if (mcid < 0) {
        fprintf(stderr, "couldn't find multicast group %s\n", group_name);
        abort();
    }

    ret = nl_socket_add_membership(nl_sock, mcid);

    if (ret) {
        fprintf(stderr, "couldn't add membership for group %s\n", group_name);
        abort();
    }
}

static void handle_info_bss_ies(const unsigned char *data, size_t length, Nl80211::InterfaceInfo &info) noexcept
{
    while (length >= 2 && length >= data[1]) {
        unsigned char t = data[0];
        size_t l = data[1];

        if (t == 0) { // SSID
            memcpy(info.ssid, data + 2, l);
            info.ssid_length = l;

            for (size_t i = 0; i < l; i++) {
                if (std::isprint(info.ssid[i]) && info.ssid[i] != '"') {
                    info.ssid_filtered[i] = info.ssid[i];
                } else {
                    info.ssid[i] = '?';
                }
            }
            info.ssid_filtered[l] = '\0';
        }

        length -= l + 2;
        data += l + 2;
    }
}

static void handle_info_bss(struct nlattr **tb, Nl80211::InterfaceInfo &info)
{
    struct nlattr *bss[NL80211_BSS_MAX + 1];

    if (nla_parse_nested(bss, NL80211_BSS_MAX,
                         tb[NL80211_ATTR_BSS],
                         bss_policy)) 
    {
        fprintf(stderr, "failed to parse nested attributes!\n");
        abort();
    }
    
    if (!bss[NL80211_BSS_STATUS] || !bss[NL80211_BSS_BSSID]) {
        return;
    }

    if (nla_len(bss[NL80211_BSS_BSSID]) != ETH_ALEN) {
        fprintf(stderr, "unexpected BSSID length\n");
        abort();
    }

    memcpy(info.mac, nla_data(bss[NL80211_BSS_BSSID]), ETH_ALEN);

    info.ssid_length = 0;
    if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
        handle_info_bss_ies(
            (unsigned char * ) nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
            nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
            info
        );
    }

    info.connected = true;
}

static void handle_info_sta(struct nlattr **tb, Nl80211::InterfaceInfo &info) {
    struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
        
    if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
                         tb[NL80211_ATTR_STA_INFO], sta_policy))
    {
        fprintf(stderr, "failed to parse nested attributes!\n");
        abort();
    }

    info.signal_strength = 0;
    if (sinfo[NL80211_STA_INFO_SIGNAL]) {
        info.signal_strength = (int8_t) nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);
    }
}

// libnl-3 callbacks
static int handle_event(struct nl_msg *msg, void *arg)
{
    Nl80211 *inst = (Nl80211 *) arg;
    inst->__handle_event(msg);
    return NL_OK;
}

static int handle_info(struct nl_msg *msg, void *arg)
{
    Nl80211::InterfaceInfo *info = (Nl80211::InterfaceInfo *) arg;
    struct genlmsghdr *gnlh = (struct genlmsghdr *) nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (tb[NL80211_ATTR_BSS]) {
        handle_info_bss(tb, *info);
    }
    if (tb[NL80211_ATTR_STA_INFO]) {
        handle_info_sta(tb, *info);
    }

    return NL_OK;
}

Nl80211::Nl80211(EventLoop &event_loop) {
    create_event_sock(event_loop);
    create_info_sock();
}

Nl80211::~Nl80211() {
    nl_cb_put(nl_event_cb);
    nl_socket_free(nl_event_sock);

    nl_cb_put(nl_info_cb);
    nl_cb_put(nl_info_s_cb);
    nl_socket_free(nl_info_sock);
}

void Nl80211::create_event_sock(EventLoop &event_loop) {
    nl_event_sock = nl_socket_alloc();

    if (!nl_event_sock) {
        fprintf(stderr, "couldn't allocate netlink socket\n");
        abort();
    }

    nl_socket_set_buffer_size(nl_event_sock, 8192, 8192);

    if (genl_connect(nl_event_sock)) {
        fprintf(stderr, "couldn't connect to netlink socket\n");
        abort();
    }

    add_to_multicast_group(nl_event_sock, "config");
    add_to_multicast_group(nl_event_sock, "scan");
    add_to_multicast_group(nl_event_sock, "regulatory");
    add_to_multicast_group(nl_event_sock, "mlme");
    add_to_multicast_group(nl_event_sock, "vendor");

    nl_socket_set_nonblocking(nl_event_sock);

    nl_event_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!nl_event_cb) {
        fprintf(stderr, "failed to allocate libnl callback\n");
        abort();
    }

    nl_cb_set(nl_event_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(nl_event_cb, NL_CB_VALID, NL_CB_CUSTOM, ::handle_event, (void *) this);
    
    event_loop.add_fd(this, nl_socket_get_fd(nl_event_sock));
}

void Nl80211::create_info_sock() {
    nl_info_sock = nl_socket_alloc();

    if (!nl_info_sock) {
        fprintf(stderr, "couldn't allocate netlink socket\n");
        abort();
    }
    
    nl_socket_set_buffer_size(nl_info_sock, 8192, 8192);

    if (genl_connect(nl_info_sock)) {
        fprintf(stderr, "couldn't connect to netlink socket\n");
        abort();
    } 

    nl_info_s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!nl_info_s_cb) {
        fprintf(stderr, "failed to allocate libnl callback\n");
        abort();
    }
    
    nl_socket_set_cb(nl_info_sock, nl_info_s_cb);
    
    nl_info_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!nl_info_cb) {
        fprintf(stderr, "failed to allocate libnl callback\n");
        abort();
    }

    info_nl80211_id = genl_ctrl_resolve(nl_info_sock, "nl80211");
    if (info_nl80211_id < 0) {
        fprintf(stderr, "couldn't resolve nl80211\n");
        abort();
    }
}

void Nl80211::descriptor_ready() noexcept {
    int res = nl_recvmsgs(nl_event_sock, nl_event_cb);

    if (res != 0) {
        fprintf(stderr, "nl_recvmsgs returned %d\n", res);
        abort();
    }
}
    
void Nl80211::get_interface_info(const char *ifname, struct InterfaceInfo &info) noexcept {
    unsigned int dev_idx;
    struct nl_msg *msg;
    int res;
    
    info.connected = false;
    
    dev_idx = if_nametoindex(ifname);

    if (dev_idx == 0) {
        fprintf(stderr, "unknown ifname %s\n", ifname);
        abort();
    }

    msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "failed to allocate struct nl_msg\n");
        abort();
    }

    genlmsg_put(msg, 0, 0, info_nl80211_id, 0,
                NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, dev_idx);
    
    // callback will fill in 'info'
    nl_cb_set(nl_info_cb, NL_CB_VALID, NL_CB_CUSTOM, ::handle_info, (void *) &info);

    res = nl_send_auto_complete(nl_info_sock, msg);
    if (res < 0) {
        fprintf(stderr, "nl_send_auto_complete received %d\n", res);
        abort();
    }

    res = nl_recvmsgs(nl_info_sock, nl_info_cb);
    if (res != 0) {
        fprintf(stderr, "nl_recvmsgs returned %d\n", res);
        abort();
    }
    
    // if we found anything, request even more details
    if (info.connected) {
        genlmsg_put(msg, 0, 0, info_nl80211_id, 0,
                    NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
        NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, dev_idx);
        NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, info.mac);

        res = nl_send_auto_complete(nl_info_sock, msg);

        if (res < 0) {
            fprintf(stderr, "nl_send_auto_complete received %d\n", res);
            abort();
        }

        res = nl_recvmsgs(nl_info_sock, nl_info_cb);

        if (res != 0) {
            fprintf(stderr, "nl_recvmsgs returned %d\n", res);
            abort();
        }
    }
    nlmsg_free(msg); 
    return;
nla_put_failure:
    fprintf(stderr, "message building failed\n");
    abort();
}

void Nl80211::__handle_event(struct nl_msg *msg) noexcept {
    for (Nl80211Listener *listener : listeners) {
        listener->nl80211event(msg);
    }
}

void Nl80211::add_listener(Nl80211Listener *listener) {
    listeners.push_back(listener);
}
