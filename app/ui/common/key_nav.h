#pragma once

#include "lvgl.h"

/** Attach to focusable widgets: arrows move focus (dir-focus first, else group). */
void ui_key_nav_cb(lv_event_t *e);

void ui_obj_add_key_nav(lv_obj_t *obj);
