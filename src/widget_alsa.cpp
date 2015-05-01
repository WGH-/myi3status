#include "widget_alsa.h"

#include <algorithm>

#include <cstdio>

std::vector<Widget_ALSA*> Widget_ALSA::__instances;

static Widget_ALSA *find_matching_widget(snd_mixer_t *mixer)
{
    for (Widget_ALSA *inst : Widget_ALSA::__instances) {
        if (inst->__mixer == mixer) {
            return inst;
        }
    }
    return nullptr;
}

static Widget_ALSA *find_matching_widget(snd_mixer_elem_t *elem)
{
    for (Widget_ALSA *inst : Widget_ALSA::__instances) {
        if (inst->__elem == elem) {
            return inst;
        }
    }
    return nullptr;
}

static int elem_callback(snd_mixer_elem_t *elem, unsigned int mask)
{
    if (mask == SND_CTL_EVENT_MASK_REMOVE) {
    } else {
        if (mask & SND_CTL_EVENT_MASK_VALUE) {
            Widget_ALSA *inst = find_matching_widget(elem);
            if (inst != nullptr) {
                inst->__volume_changed(elem);
            }
        }

        if (mask & SND_CTL_EVENT_MASK_INFO) {
        }
    }

    return 0;
}

static int mixer_callback(snd_mixer_t *mixer, unsigned int mask, snd_mixer_elem_t *elem)
{
    if (mask & SND_CTL_EVENT_MASK_ADD) {
        if (strcmp(snd_mixer_selem_get_name(elem), "Master") == 0) {
            Widget_ALSA *inst = find_matching_widget(mixer);
            inst->__elem = elem;
            snd_mixer_elem_set_callback(elem, elem_callback);
        }
    }
    return 0;
}

Widget_ALSA::Widget_ALSA(EventLoop &event_loop) 
{
    __instances.push_back(this);
    
    volume = -1;

    open_mixer(event_loop);

    update_string();
}

Widget_ALSA::~Widget_ALSA()
{
    snd_mixer_free(__mixer);

    // XXX I don't really understand this shit
    //     well, destructors are never called, anyway
    __instances.erase(std::remove(__instances.begin(), __instances.end(), this), __instances.end());
}

void Widget_ALSA::open_mixer(EventLoop &event_loop)
{
    int err;

    struct snd_mixer_selem_regopt selem_regopt = {};
    selem_regopt.ver = 1;
    selem_regopt.abstract = SND_MIXER_SABSTRACT_NONE;
    selem_regopt.device = "default";

    err = snd_mixer_open(&__mixer, 0);
    assert(err == 0);

    err = snd_mixer_selem_register(__mixer, &selem_regopt, NULL);
    assert(err == 0);

    snd_mixer_set_callback(__mixer, mixer_callback);

    err = snd_mixer_load(__mixer);
    assert(err == 0);

    int nfds = snd_mixer_poll_descriptors_count(__mixer);

    std::vector<struct pollfd> pollfds;
    pollfds.resize(nfds);
    
    err = snd_mixer_poll_descriptors(__mixer, &pollfds[0], nfds);
    assert(err >= 0);

    for (const struct pollfd &pollfd : pollfds) {
        event_loop.add_fd(this, pollfd.fd);
    }
}

void Widget_ALSA::update_string() noexcept 
{
    if (volume >= 0) {
        snprintf(string, sizeof(string),
            "{\"full_text\": \"VOL: %3.0f%%\"}",
            volume * 100
        );
    } else {
        string[0] = '\0';
    }
}

const char* Widget_ALSA::get_string(bool force_update) noexcept
{
    return string;
}

void Widget_ALSA::descriptor_ready() noexcept 
{
    snd_mixer_handle_events(__mixer);
}

void Widget_ALSA::__volume_changed(snd_mixer_elem_t *elem)
{
    if (snd_mixer_selem_has_playback_volume(elem)) {
        long value, value_left, value_right;
        long pmin, pmax;

        snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
        
        snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &value_left); 
        snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &value_right); 

        value = std::max(value_left, value_right);

        volume = float(value - pmin) / float(pmax - pmin);

        update_string();
    }
}
