#include "key_nav.h"

#include "lvgl/ext/lv_dir_focus.h"

void ui_key_nav_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_key_t key = lv_event_get_key(e);
    if (lv_obj_focus_dir_by_key(obj, key)) {
        return;
    }
    lv_group_t *group = lv_obj_get_group(obj);
    if (group == NULL) {
        return;
    }
    if (key == LV_KEY_UP || key == LV_KEY_LEFT) {
        lv_group_focus_prev(group);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT) {
        lv_group_focus_next(group);
    }
}

void ui_obj_add_key_nav(lv_obj_t *obj) {
    if (obj == NULL) {
        return;
    }
    lv_obj_add_event_cb(obj, ui_key_nav_cb, LV_EVENT_KEY, NULL);
}
