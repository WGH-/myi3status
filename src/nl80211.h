#pragma once

#include <vector>

#include <linux/if_ether.h>
#include <linux/nl80211.h>

#include <netlink/netlink.h>
#include <netlink/genl/ctrl.h>

#include "event_loop.h"

extern "C" {
    extern struct nla_policy sta_policy[NL80211_STA_INFO_MAX + 1];
    extern struct nla_policy bss_policy[NL80211_BSS_MAX + 1];
}

class Nl80211Listener {
public:
    virtual void nl80211event(struct nl_msg *msg) noexcept = 0;
};

class Nl80211 : public Epollable {
    std::vector<Nl80211Listener*> listeners;

    // netlink socket for monitoring
    struct nl_sock *nl_event_sock;
    struct nl_cb *nl_event_cb;

    // netlink socket for request/response
    struct nl_sock *nl_info_sock;
    struct nl_cb *nl_info_cb, *nl_info_s_cb;
    int info_nl80211_id;
    
    // constructor helpers
    void create_event_sock(EventLoop &event_loop);
    void create_info_sock();

public:
    struct InterfaceInfo {
        InterfaceInfo() {
            connected = false;
        }

        bool connected;
        
        char mac[ETH_ALEN];

        size_t ssid_length;
        char ssid[64]; // isn't supposed to be >32, though
        char ssid_filtered[64]; // null-terminated ssid with nonprintable chars removed

        int signal_strength;
    };
        
    Nl80211(EventLoop &event_loop);
    ~Nl80211();

    void get_interface_info(const char *, struct InterfaceInfo &info) noexcept;

    virtual void descriptor_ready() noexcept override;
    void add_listener(Nl80211Listener *listener);
    
    // callbacks (they're called internally)
    void __handle_event(struct nl_msg *msg) noexcept;
};
