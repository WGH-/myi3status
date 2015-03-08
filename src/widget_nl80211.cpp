#include "widget_nl80211.h"

#include <cstdio>
#include <cstdlib>
#include <cctype>

// XXX hack (netlink/route/link.h conflicts with net/if.h)
extern "C" char *if_indextoname (unsigned int __ifindex, char *__ifname);

#include <linux/nl80211.h>

#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/route/link.h>

#define DISCONNECTED_COLOR "#FF0000"

Widget_nl80211::Widget_nl80211(Nl80211 &nl80211, Rtnetlink &rtnetlink, const char *ifname)
    : nl80211(nl80211), rtnetlink(rtnetlink)
{
    this->ifname = ifname;
    
    nl80211.add_listener(this);
    rtnetlink.add_link_listener(this);

    update_string();
}

void Widget_nl80211::update_string(struct rtnl_link *link) noexcept {
    rtnetlink.get_link_info(ifname, link_info);
    nl80211.get_interface_info(ifname, info);

    if (info.connected) {
        snprintf(string, sizeof(string), 
            "{\"full_text\": \"%s\"}",
            info.ssid_filtered
        );
    } else {
        const char *status = "disconnected";

        if (!(link_info.iff_flags & IFF_UP)) {
            status = "down";
        }

        snprintf(string, sizeof(string), 
            "{"
                "\"full_text\": \"(%s)\","
                "\"color\": \"" DISCONNECTED_COLOR "\" "
            "}",
            status
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

void Widget_nl80211::link_event(struct rtnl_link *link) noexcept {
    if (strcmp(rtnl_link_get_name(link), ifname) == 0) {
        update_string();
    }
}
