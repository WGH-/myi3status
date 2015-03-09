#include "widget_ip.h"

#include <arpa/inet.h>

// XXX hack (netlink/route/link.h conflicts with net/if.h)
extern "C" char *if_indextoname (unsigned int __ifindex, char *__ifname);

#include <netlink/route/addr.h>

#define IP_GOOD_COLOR "#00aa00"

Widget_IP::Widget_IP(Rtnetlink &rtnetlink, const char *ifname)
    : rtnetlink(rtnetlink)
{
    this->ifname = ifname;
    
    rtnetlink.add_addr_listener(this);
    rtnetlink.force_addr_update();

    update_string();
}

void Widget_IP::update_data(struct rtnl_addr *addr) noexcept {
    rtnetlink.fill_addr_info(addr_info, addr);
}

void Widget_IP::update_string() noexcept {
    if (addr_info.invalid) {
        string[0] = '\0';
    } else {
        char buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr_info.addr, buffer, sizeof(buffer));

        snprintf(string, sizeof(string), 
            "{"
                "\"full_text\": \"%s\","
                "\"color\": \"" IP_GOOD_COLOR "\""
            "}",
            buffer
        );
    }
}

const char* Widget_IP::get_string(void) const noexcept
{
    return string;
}

void Widget_IP::addr_event(struct rtnl_addr *addr) noexcept
{
    char ifname[100];
    if_indextoname(rtnl_addr_get_ifindex(addr), ifname);

    if (strcmp(ifname, this->ifname) == 0) {
        update_data(addr);
        update_string();
    }
}