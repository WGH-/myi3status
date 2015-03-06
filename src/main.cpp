#include <cstdio>
#include <unistd.h>

#include "event_loop.h"

#include "widget_time.h"


int main(void) {
    EventLoop event_loop;

#define NEW_WIDGET(CLASS, VARNAME, ...) \
    CLASS VARNAME{event_loop, __VA_ARGS__}; \
    event_loop.add_widget(&VARNAME);

    NEW_WIDGET(WidgetTime, widget_time);

    event_loop.run();

    return 0;
}
