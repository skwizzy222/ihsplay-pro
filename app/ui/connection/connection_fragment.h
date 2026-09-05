#pragma once

#include "lvgl.h"

extern const lv_fragment_class_t connection_fragment_class;

void connection_fragment_set_title(lv_fragment_t *self, const char *title);

void connection_fragment_submit_stream_pin(lv_fragment_t *self, const char *pin);

void connection_fragment_cancel(lv_fragment_t *self);
