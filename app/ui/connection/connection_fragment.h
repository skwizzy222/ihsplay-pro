#pragma once

#include "lvgl.h"
#include <ihslib.h>

typedef struct connection_launch_args_t {
    IHS_HostInfo host;
    IHS_StreamInterface stream_interface;
} connection_launch_args_t;

extern const lv_fragment_class_t connection_fragment_class;

void connection_fragment_set_title(lv_fragment_t *self, const char *title);

void connection_fragment_submit_stream_pin(lv_fragment_t *self, const char *pin);

void connection_fragment_cancel(lv_fragment_t *self);
