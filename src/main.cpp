#include <cstdio>
#include <unistd.h>

#include "event_loop.h"
#include "timer.h"
#include "timer_oneshot.h"
#include "nl80211.h"
#include "rtnetlink.h"
#include "udev.h"

#include "widget_ip.h"
#include "widget_nl80211.h"
#include "widget_time.h"
#include "widget_battery.h"
#include "widget_wattage.h"
#include "widget_ac.h"
#include "widget_pulse.h"
#include "widget_mem.h"

#include "click_manager.h"

int main(void) {
    EventLoop event_loop;

    UdevMonitor udev_monitor(event_loop);
    Nl80211 nl80211(event_loop);
    Rtnetlink rtnetlink(event_loop);
    TimerManager timer_manager(event_loop);
    OneshotTimerManager oneshot_timer_manager(event_loop);
    ClickManager click_manager(event_loop);

#define NEW_WIDGET(CLASS, VARNAME, ...) \
    CLASS VARNAME{__VA_ARGS__}; \
    event_loop.add_widget(&VARNAME)

    NEW_WIDGET(Widget_IP, widget_ctfvpn_ip, rtnetlink, "ctfvpn", "#ff7f00");
    NEW_WIDGET(Widget_IP, widget_wlp3s0_ip, rtnetlink, "wlp3s0", "#008b8b");
    NEW_WIDGET(Widget_IP, widget_enp0s25_ip, rtnetlink, "enp0s25", "#8b8b00");
    NEW_WIDGET(Widget_nl80211, widget_wlp3s0, timer_manager, oneshot_timer_manager, click_manager, nl80211, rtnetlink, "wlp3s0", 5000);
    NEW_WIDGET(WidgetMem, widget_mem, timer_manager, 1000);
    NEW_WIDGET(WidgetAC, widget_ac, event_loop, udev_monitor, "AC");
    NEW_WIDGET(WidgetWattage, widget_wattage, timer_manager, {"BAT0", "BAT1"}, 2500);
    NEW_WIDGET(WidgetBattery, widget_battery0, timer_manager, "BAT0", 2500);
    NEW_WIDGET(WidgetBattery, widget_battery1, timer_manager, "BAT1", 2500);
    NEW_WIDGET(Widget_Pulse, widget_pulse, event_loop, oneshot_timer_manager, "@DEFAULT_SINK@");
    NEW_WIDGET(WidgetTime, widget_time, event_loop);

    event_loop.run();

    return 0;
}
