#include "click_manager.h"

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#include <yajl/yajl_parse.h>

enum i3bar_parse_state {
    I3BAR_STATE_NONE = 0,
    I3BAR_STATE_ARRAY,
    I3BAR_STATE_OBJECT,
};

enum i3bar_json_key {
    I3BAR_KEY_UNKNOWN = 0,
    I3BAR_KEY_NAME,
    I3BAR_KEY_INSTANCE,
    I3BAR_KEY_BUTTON,
    I3BAR_KEY_X,
    I3BAR_KEY_Y,
};

struct i3bar_event {
    const char *name;
    const char *instance;
    int button;
    long x, y;
};

struct i3bar_parse_ctx {
    enum i3bar_parse_state state;
    enum i3bar_json_key current_key;

    struct i3bar_event current_event;

    const char *errstring;

    ClickManager *object;
};

static int handle_gnull(void * p)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    return 1;
}
static int handle_gboolean(void * p, int boolean)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    return 1;
}
static int handle_glong(void * p, long long val)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    if (ctx->state == I3BAR_STATE_NONE) {
        ctx->errstring = "Unexpected top-level number";
        return 0;
    }

    if (ctx->state == I3BAR_STATE_ARRAY) {
        ctx->errstring = "Array should contain objects, got number";
        return 0;
    }

    assert(ctx->state == I3BAR_STATE_OBJECT);

    switch (ctx->current_key) {
    case I3BAR_KEY_BUTTON:
        ctx->current_event.button = val;
        break;
    case I3BAR_KEY_X:
        ctx->current_event.x = val;
        break;
    case I3BAR_KEY_Y:
        ctx->current_event.y = val;
        break;
    case I3BAR_KEY_NAME:
    case I3BAR_KEY_INSTANCE:
        ctx->errstring = "Expected string, got number";
        return 0;
    case I3BAR_KEY_UNKNOWN:
        break;
    }
}
static int handle_gstring(void * p, const unsigned char * stringVal,
                           size_t stringLen)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    if (ctx->state == I3BAR_STATE_NONE) {
        ctx->errstring = "Unexpected top-level string";
        return 0;
    }

    if (ctx->state == I3BAR_STATE_ARRAY) {
        ctx->errstring = "Array should contain objects, got string";
        return 0;
    }

    assert(ctx->state == I3BAR_STATE_OBJECT);

    switch (ctx->current_key) {
    case I3BAR_KEY_NAME:
        assert(ctx->current_event.name == NULL);
        ctx->current_event.name = strndup((const char *)stringVal, stringLen);
        break;
    case I3BAR_KEY_INSTANCE:
        assert(ctx->current_event.instance == NULL);
        ctx->current_event.instance = strndup((const char *)stringVal, stringLen);
        break;
    case I3BAR_KEY_BUTTON:
    case I3BAR_KEY_X:
    case I3BAR_KEY_Y:
        ctx->errstring = "Expected number, got string";
        return 0;
    case I3BAR_KEY_UNKNOWN:
        break;
    }

    return 1;
}
static int handle_gmap_key(void * p, const unsigned char * stringVal,
                            size_t stringLen)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    assert(ctx->state == I3BAR_STATE_OBJECT);

    if (strncmp((const char *)stringVal, "name", stringLen) == 0) {
        ctx->current_key = I3BAR_KEY_NAME;
    } else if (strncmp((const char *)stringVal, "instance", stringLen) == 0) {
        ctx->current_key = I3BAR_KEY_INSTANCE;
    } else if (strncmp((const char *)stringVal, "button", stringLen) == 0) {
        ctx->current_key = I3BAR_KEY_BUTTON;
    } else if (strncmp((const char *)stringVal, "x", stringLen) == 0) {
        ctx->current_key = I3BAR_KEY_X;
    } else if (strncmp((const char *)stringVal, "y", stringLen) == 0) {
        ctx->current_key = I3BAR_KEY_Y;
    } else {
        ctx->current_key = I3BAR_KEY_UNKNOWN;
    }

    return 1;
}
static int handle_gstart_map(void * p)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    if (ctx->state == I3BAR_STATE_NONE) {
        ctx->errstring = "Unexpected top-most object";
        return 0;
    }

    if (ctx->state == I3BAR_STATE_OBJECT) {
        ctx->errstring = "Unexpected nested object";
        return 0;
    }

    assert(ctx->state == I3BAR_STATE_ARRAY);

    ctx->state = I3BAR_STATE_OBJECT;

    ctx->current_event.name = NULL;
    ctx->current_event.instance = NULL;
    ctx->current_event.button = 0;
    ctx->current_event.x = 0;
    ctx->current_event.y = 0;

    return 1;
}
static int handle_gend_map(void * p)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    assert(ctx->state == I3BAR_STATE_OBJECT);

    ctx->state = I3BAR_STATE_ARRAY;

    ctx->object->click_handler(
        ctx->current_event.name,
        ctx->current_event.instance,
        ctx->current_event.button,
        ctx->current_event.x,
        ctx->current_event.y
    );

    free((void *)ctx->current_event.name);
    free((void *)ctx->current_event.instance);

    return 1;
}
static int handle_gstart_array(void * p)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    if (ctx->state != I3BAR_STATE_NONE) {
        ctx->errstring = "Unexpected nested array";
        return 0;
    }

    ctx->state = I3BAR_STATE_ARRAY;

    return 1;
}
static int handle_gend_array(void * p)
{
    auto ctx = (struct i3bar_parse_ctx *) p;

    assert(ctx->state == I3BAR_STATE_ARRAY);

    ctx->errstring = "Endless array has ended";

    return 0;
}


static yajl_callbacks callbacks = {
    handle_gnull,
    handle_gboolean,
    handle_glong,
    NULL,
    NULL,
    handle_gstring,
    handle_gstart_map,
    handle_gmap_key,
    handle_gend_map,
    handle_gstart_array,
    handle_gend_array
};

ClickManager::ClickManager(EventLoop &event_loop)
{
    int flags;
    struct i3bar_parse_ctx *ctx;

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    event_loop.add_fd(this, STDIN_FILENO);

    ctx = (struct i3bar_parse_ctx *) calloc(1, sizeof(struct i3bar_parse_ctx));
    assert(ctx != NULL);
    ctx->object = this;

    private_data = ctx;

    hand = yajl_alloc(&callbacks, NULL, (void *) private_data);
    assert(hand != NULL);

    read_input();
}

ClickManager::~ClickManager()
{
    yajl_free(hand);
    free(private_data);
}

void ClickManager::descriptor_ready()
{
    read_input();
}

void ClickManager::read_input()
{
    while (1) {
        char buffer[512];
        ssize_t res;
        yajl_status stat;

        res = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (res == 0) {
            break;
        }

        if (res < 0) {
            if (errno == EAGAIN) {
                break;
            }
            exit(1);
        }

        stat = yajl_parse(hand, (const unsigned char *)buffer, res);

        if (stat == yajl_status_client_canceled) {
            fprintf(stderr, "Client aborted\n");
            exit(1);
        }

        if (stat != yajl_status_ok) {
            fprintf(stderr, "NOT EXPECTED");
            exit(1);
        }
    }
}

void ClickManager::click_handler(const char *name, const char *instance, int button, long x, long y)
{
    if (name == nullptr || instance == nullptr) {
        return;
    }

    auto key = std::make_pair(name, instance);
    auto it = clickables.find(key);
    if (it != clickables.end()) {
        it->second->click_handler(button);
    }
}

void ClickManager::register_clickable(WidgetClickable *clickable)
{
    auto key = std::make_pair(clickable->clickable_name(), clickable->clickable_instance());
    clickables[key] = clickable;
}
