#include <cstdio>
#include <unistd.h>

#include "event_loop.h"
#include "nl80211.h"
#include "rtnetlink.h"
#include "udev.h"

#include "widget_ip.h"
#include "widget_nl80211.h"
#include "widget_time.h"
#include "widget_battery.h"
#include "widget_wattage.h"
#include "widget_ac.h"
#include "widget_alsa.h"

int main(void) {
    EventLoop event_loop;

    UdevMonitor udev_monitor(event_loop);
    Nl80211 nl80211(event_loop);
    Rtnetlink rtnetlink(event_loop);

#define NEW_WIDGET(CLASS, VARNAME, ...) \
    CLASS VARNAME{__VA_ARGS__}; \
    event_loop.add_widget(&VARNAME)

    NEW_WIDGET(Widget_IP, widget_ctfvpn_ip, rtnetlink, "ctfvpn", "#ff7f00");
    NEW_WIDGET(Widget_IP, widget_wlp3s0_ip, rtnetlink, "wlp3s0", "#008b8b");
    NEW_WIDGET(Widget_nl80211, widget_wlp3s0, nl80211, rtnetlink, "wlp3s0");
    NEW_WIDGET(WidgetAC, widget_ac, event_loop, udev_monitor, "AC");
    NEW_WIDGET(WidgetWattage, widget_wattage, event_loop, {"BAT0", "BAT1"});
    NEW_WIDGET(WidgetBattery, widget_battery0, event_loop, "BAT0");
    NEW_WIDGET(WidgetBattery, widget_battery1, event_loop, "BAT1");
    NEW_WIDGET(Widget_ALSA, wiget_alsa, event_loop);
    NEW_WIDGET(WidgetTime, widget_time, event_loop);

    event_loop.run();

    return 0;
}
