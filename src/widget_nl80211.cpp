#include "widget_nl80211.h"

#include <cstdio>
#include <cstdlib>
#include <cctype>

// XXX hack (netlink/route/link.h conflicts with net/if.h)
extern "C" char *if_indextoname (unsigned int __ifindex, char *__ifname);

#include <linux/nl80211.h>
#include <linux/if.h>

#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/route/link.h>


#define DISCONNECTED_COLOR "#FF0000"

static const char *signal_strengths[] = {
    " ","▁","▂","▃","▄","▅","▆","▇","█"
};

static int signal_level_from_rssi(int rssi, int levels)
{
    static const int MIN_RSSI = -95;
    static const int MAX_RSSI = -65;

    if (rssi <= MIN_RSSI) {
        return 0;
    } else if (rssi >= MAX_RSSI) {
        return levels-1;
    } else {
        float delta = MAX_RSSI - MIN_RSSI;

        return int(float(rssi - MIN_RSSI) * (levels-1) / delta);
    }
}

Widget_nl80211::Widget_nl80211(
        TimerManager &timer_manager,
        OneshotTimerManager &oneshot_timer_manager,
        ClickManager &click_manager,
        Nl80211 &nl80211,
        Rtnetlink &rtnetlink,
        const char *ifname,
        unsigned long signal_poll_interval_ms)
    : timer_manager(timer_manager), oneshot_timer_manager(oneshot_timer_manager), nl80211(nl80211), rtnetlink(rtnetlink), signal_poll_interval_ms(signal_poll_interval_ms)
{
    this->ifname = ifname;

    nl80211.add_listener(this);
    rtnetlink.add_link_listener(this);
    click_manager.register_clickable(this);

    expanded_view = true;
    oneshot_timer_manager.schedule_after(std::chrono::seconds(1), this);

    update_string();
}

void Widget_nl80211::update_string() noexcept {
    rtnetlink.get_link_info(ifname, link_info);
    nl80211.get_interface_info(ifname, info);

    if (info.connected) {
        int level;

        level = signal_level_from_rssi(info.signal_strength, sizeof(signal_strengths) / sizeof(signal_strengths[0]));

        if (expanded_view) {
            snprintf(string, sizeof(string),
                "{\"full_text\": \"%s %s\", \"name\":\"%s\", \"instance\":\"%s\"}",
                signal_strengths[level],
                info.ssid_filtered,
                clickable_name(),
                clickable_instance()
            );
        } else {
            snprintf(string, sizeof(string),
                "{\"full_text\": \"%s\", \"name\":\"%s\", \"instance\":\"%s\"}",
                signal_strengths[level],
                clickable_name(),
                clickable_instance()
            );
        }

        timer_manager.register_monotonic_listener(this, std::chrono::milliseconds(signal_poll_interval_ms));
    } else {
        if (link_info.iff_flags & IFF_UP) {
            snprintf(string, sizeof(string),
                "{"
                    "\"full_text\": \"(%s)\","
                    "\"color\": \"" DISCONNECTED_COLOR "\","
                    "\"name\":\"%s\", \"instance\":\"%s\""
                "}",
                expanded_view ? "disconnected" : "d",
                clickable_name(), clickable_instance()
            );
        } else {
            // network is down, don't draw anything
            strncat(string, "", sizeof(string));
        }

        timer_manager.unregister_monotonic_listener(this);
    }
}

const char * Widget_nl80211::get_string(bool force_update) noexcept {
    if (force_update && info.connected) {
        // update signal strength
        update_string();
    }
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
            expanded_view = true;
            oneshot_timer_manager.schedule_after(std::chrono::seconds(5), this);
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

void Widget_nl80211::timer_ready() noexcept {
    /*
     * The timer is used solely to poll signal level.
     * If we aren't connected, don't do anything.
     */

    if (info.connected) {
        update_string();
    }
}

/*
 * Clickable interface
 */

const char * Widget_nl80211::clickable_name() const noexcept {
    return "nl80211";
}

const char * Widget_nl80211::clickable_instance() const noexcept {
    return ifname;
}

void Widget_nl80211::click_handler(int button) noexcept {
    expanded_view = true;
    oneshot_timer_manager.schedule_after(std::chrono::seconds(3), this);
    update_string();
}

/*
 * One-shot timer
 */
void Widget_nl80211::oneshot_timer_ready() noexcept {
    expanded_view = false;
    update_string();
}
