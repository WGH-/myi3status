#pragma once

#include "event_loop.h"

#include <yajl/yajl_parse.h>

#include <map>

class WidgetClickable;

class ClickManager : public Epollable {
public:
    ClickManager(EventLoop &event_loop);
    ~ClickManager();

    virtual void descriptor_ready() noexcept;

    void click_handler(const char *name, const char *instance, int button, long x, long y);

    void register_clickable(WidgetClickable *);
protected:
    yajl_handle hand;
    void *private_data;

    void read_input();

    typedef std::pair<std::string, std::string> clickable_key_t;
    std::map<clickable_key_t, WidgetClickable*> clickables;
};

class WidgetClickable {
public:
    virtual void click_handler(int button) = 0;

    virtual const char* clickable_name() const noexcept = 0;
    virtual const char* clickable_instance() const noexcept = 0;
};
