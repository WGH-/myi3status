#include "widget_pulse.h"

#include <cstdio>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <signal.h>

static void context_state_callback(
        pa_context *c,
        void *userdata)
{
    Widget_Pulse *wp = (Widget_Pulse*) userdata;

    wp->context_state_callback(c);
}

static void context_subscribe_callback(
        pa_context *c,
        pa_subscription_event_type_t t,
        uint32_t idx,
        void *userdata)
{
    Widget_Pulse *wp = (Widget_Pulse*) userdata;

    wp->context_subscribe_callback(c, t, idx);
}

static void get_sink_volume_callback(
        pa_context *c,
        const pa_sink_info *i,
        int is_last,
        void *userdata)
{
    Widget_Pulse *wp = (Widget_Pulse*) userdata;

    wp->get_sink_volume_callback(
        c, i, is_last);
}

Widget_Pulse::Widget_Pulse(EventLoop &event_loop,
                           const char *sink_name)
{
    this->sink_name = sink_name;

    volume = PA_VOLUME_INVALID;
    update_string();

    fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    event_loop.add_fd(this, fd);

    worker_thread = std::thread(&Widget_Pulse::worker_thread_function, this);
}

Widget_Pulse::~Widget_Pulse()
{
    // TODO actually destroy the thread
    worker_thread.join();
    close(fd);
}

void Widget_Pulse::worker_thread_function()
{
    pa_mainloop *m;
    pa_mainloop_api *api;
    pa_context *c;
    int res;

    prctl(PR_SET_NAME, "PA main loop");

    // block USR1
    {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);

        res = sigprocmask(SIG_BLOCK, &mask, NULL);
        assert(res == 0);
    }

    m = pa_mainloop_new();
    assert(m != nullptr);

    api = pa_mainloop_get_api(m);
    assert(api != nullptr);

    c = pa_context_new_with_proplist(api, NULL, NULL);
    assert(c != nullptr);

    pa_context_set_state_callback(c, ::context_state_callback, this);

    res = pa_context_connect(c, NULL, PA_CONTEXT_NOFLAGS, NULL);
    assert(res == 0);

    res = pa_mainloop_run(m, nullptr);
    assert(false);
}

void Widget_Pulse::context_state_callback(pa_context *c)
{
    pa_operation *o = nullptr;

    if (pa_context_get_state(c) == PA_CONTEXT_READY) {
        pa_context_set_subscribe_callback(
            c, ::context_subscribe_callback, this);

        pa_subscription_mask_t mask = PA_SUBSCRIPTION_MASK_SINK;

        o = pa_context_subscribe(
            c, mask, NULL, NULL);
        assert(o != nullptr);
        pa_operation_unref(o);

        update_volume(c);
    }
}

void Widget_Pulse::context_subscribe_callback(
        pa_context *c,
        pa_subscription_event_type_t t,
        uint32_t idx)
{
    update_volume(c);
}

void Widget_Pulse::get_sink_volume_callback(
        pa_context *c,
        const pa_sink_info *i,
        int is_last)
{
    if (is_last != 0) return;

    assert(is_last == 0);
    assert(i != nullptr);

    this->volume = pa_cvolume_avg(&i->volume);
    
    notify_eventfd();
}

void Widget_Pulse::update_volume(pa_context *c)
{
    pa_operation *o = nullptr;
    o = pa_context_get_sink_info_by_name(c, sink_name, ::get_sink_volume_callback, this);
    pa_operation_unref(o);
}

void Widget_Pulse::notify_eventfd()
{
    uint64_t val = 1;
    ssize_t res;

    res = write(fd, &val, sizeof(val));
    assert(res == sizeof(val));
}

void Widget_Pulse::update_string() noexcept
{
    pa_volume_t volume = this->volume;

    if (PA_VOLUME_IS_VALID(volume)) {
        char volume_string[PA_VOLUME_SNPRINT_MAX];
        pa_volume_snprint(volume_string, sizeof(volume_string), volume);

        snprintf(string, sizeof(string),
            "{\"full_text\": \"VOL: %s\"}",
            volume_string
        );
    } else {
        string[0] = '\0';
    }
}

const char* Widget_Pulse::get_string(bool force_update) noexcept
{
    return string;
}

void Widget_Pulse::descriptor_ready() noexcept
{
    uint64_t val;
    ssize_t res = read(fd, &val, sizeof(val));
    if (res < 0) {
        if (errno != EAGAIN) {
            assert(false);
        }
        return;
    }

    update_string();
}

