#include <cstdio>
#include <unistd.h>

#include "event_loop.h"

#include "widget_time.h"
#include "widget_battery.h"
#include "widget_ac.h"

int main(void) {
    EventLoop event_loop;

#define NEW_WIDGET(CLASS, VARNAME, ...) \
    CLASS VARNAME{event_loop, __VA_ARGS__}; \
    event_loop.add_widget(&VARNAME)

    NEW_WIDGET(WidgetAC, widget_ac, "AC");
    NEW_WIDGET(WidgetBattery, widget_battery0, "BAT0");
    NEW_WIDGET(WidgetBattery, widget_battery1, "BAT1");
    NEW_WIDGET(WidgetTime, widget_time);

    event_loop.run();

    return 0;
}
