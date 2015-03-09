#pragma once

#include <alsa/asoundlib.h>

#include "event_loop.h"

class Widget_ALSA : public Widget, Epollable {

    void update_string() noexcept;
    char string[64];

    // constructor helper
    void open_mixer(EventLoop &event_loop);

    float volume; 
public:
    Widget_ALSA(EventLoop &event_loop);
    ~Widget_ALSA();

    virtual const char* get_string(bool force_update) noexcept override;
    virtual void descriptor_ready() noexcept override;
    
    // for internal use 
    // (they're public for hackish reasons of interfacing with C callbacks)
    snd_mixer_t *__mixer;
    snd_mixer_elem_t *__elem;
    void __volume_changed(snd_mixer_elem_t *elem);

    // need to use this hack because
    // alsa doesn't allow us to associate some kind of
    // void* with its callbacks
    static std::vector<Widget_ALSA*> __instances;

};
