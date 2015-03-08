#include "widget_nl80211.h"

#include <cstdio>
#include <cstdlib>
#include <cctype>

#include <net/if.h>

#include <linux/nl80211.h>

#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define DISCONNECTED_COLOR "#FF0000"

Widget_nl80211::Widget_nl80211(Nl80211 &nl80211, const char *ifname)
    : nl80211(nl80211)
{
    this->ifname = ifname;
    
    nl80211.add_listener(this);

    update_string();
}

void Widget_nl80211::update_string() noexcept {
    nl80211.get_interface_info(ifname, info);

    if (info.connected) {
        snprintf(string, sizeof(string), 
            "{\"full_text\": \"%s\"}",
            info.ssid_filtered
        );
    } else {
        snprintf(string, sizeof(string), 
            "{"
                "\"full_text\": \"(disconnected)\","
                "\"color\": \"" DISCONNECTED_COLOR "\" "
            "}"
        );
    }
}

const char * Widget_nl80211::get_string() const noexcept {
    return string;
}

void Widget_nl80211::nl80211event(struct nl_msg *msg) noexcept {
    struct genlmsghdr *gnlh = (struct genlmsghdr *) nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    char ifname[100];

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_IFINDEX]) {
        // usually events like regulatory domain change fall here
        return;
    }

    if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), ifname);

    if (strncmp(ifname, this->ifname, sizeof(ifname)) != 0) {
        // some unknown interface
        return;
    }

    switch (gnlh->cmd) {
        case NL80211_CMD_CONNECT: 
        case NL80211_CMD_DISCONNECT:
        case NL80211_CMD_ROAM:
            update_string();
            break;
        default:
            break;
    }
}
