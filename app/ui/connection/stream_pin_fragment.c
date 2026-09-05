#include "stream_pin_fragment.h"

#include "ui/app_ui.h"
#include "connection_fragment.h"
#include "lvgl/ext/lv_dir_focus.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef struct stream_pin_fragment_t {
    lv_fragment_t base;
    app_t *app;
    char pin[5];
    int cursor;
    lv_obj_t *pin_label;
    lv_group_t *group;
    lv_obj_t *digit_btns[10];
    lv_obj_t *delete_btn;
    lv_obj_t *submit_btn;
    lv_obj_t *cancel_btn;
} stream_pin_fragment_t;

static void keypad_key(lv_event_t *e);

static void wire_dir_focus(stream_pin_fragment_t *fragment);

static void ctor(lv_fragment_t *self, void *arg);

static void dtor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static void refresh_pin_label(stream_pin_fragment_t *fragment);

static void digit_clicked(lv_event_t *e);

static void backspace_clicked(lv_event_t *e);

static void submit_clicked(lv_event_t *e);

static void cancel_clicked(lv_event_t *e);

const lv_fragment_class_t stream_pin_fragment_class = {
        .constructor_cb = ctor,
        .destructor_cb = dtor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .instance_size = sizeof(stream_pin_fragment_t)
};

static void ctor(lv_fragment_t *self, void *arg) {
    stream_pin_fragment_t *fragment = (stream_pin_fragment_t *) self;
    app_ui_fragment_args_t *args = arg;
    fragment->app = args->app;
    memset(fragment->pin, 0, sizeof(fragment->pin));
    fragment->cursor = 0;
}

static void dtor(lv_fragment_t *self) {
    (void) self;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    stream_pin_fragment_t *fragment = (stream_pin_fragment_t *) self;
    lv_obj_t *obj = lv_obj_create(container);
    lv_obj_set_style_pad_gap(obj, LV_DPX(12), 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_t *hint = lv_label_create(obj);
    lv_label_set_text(hint, "Введите PIN с экрана Steam на ПК");
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    fragment->pin_label = lv_label_create(obj);
    lv_obj_set_style_text_font(fragment->pin_label, fragment->app->ui->font.huge, 0);
    refresh_pin_label(fragment);

    lv_obj_t *pad = lv_obj_create(obj);
    lv_obj_set_size(pad, LV_DPX(360), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_gap(pad, LV_DPX(8), 0);
    lv_obj_set_style_bg_opa(pad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pad, 0, 0);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(pad, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, true);

    static const char *digits[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int i = 0; i < 10; i++) {
        lv_obj_t *btn = lv_btn_create(pad);
        lv_obj_set_size(btn, LV_DPX(72), LV_DPX(56));
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, digits[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, digit_clicked, LV_EVENT_CLICKED, fragment);
        lv_obj_add_event_cb(btn, keypad_key, LV_EVENT_KEY, NULL);
        lv_obj_set_user_data(btn, (void *) (intptr_t) digits[i][0]);
        lv_group_add_obj(fragment->group, btn);
        fragment->digit_btns[i] = btn;
    }

    lv_obj_t *actions = lv_obj_create(obj);
    lv_obj_set_size(actions, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_gap(actions, LV_DPX(12), 0);
    lv_obj_set_style_bg_opa(actions, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(actions, 0, 0);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);

    fragment->delete_btn = lv_btn_create(actions);
    lv_obj_t *backspace_label = lv_label_create(fragment->delete_btn);
    lv_label_set_text(backspace_label, "Стереть");
    lv_obj_center(backspace_label);
    lv_obj_add_event_cb(fragment->delete_btn, backspace_clicked, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->delete_btn, keypad_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(fragment->group, fragment->delete_btn);

    fragment->submit_btn = lv_btn_create(actions);
    lv_obj_t *submit_label = lv_label_create(fragment->submit_btn);
    lv_label_set_text(submit_label, "Подключить");
    lv_obj_center(submit_label);
    lv_obj_add_event_cb(fragment->submit_btn, submit_clicked, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->submit_btn, keypad_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(fragment->group, fragment->submit_btn);

    fragment->cancel_btn = lv_btn_create(actions);
    lv_obj_t *cancel_label = lv_label_create(fragment->cancel_btn);
    lv_label_set_text(cancel_label, "Отмена");
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(fragment->cancel_btn, cancel_clicked, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->cancel_btn, keypad_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(fragment->group, fragment->cancel_btn);

    wire_dir_focus(fragment);
    return obj;
}

static void wire_dir_focus(stream_pin_fragment_t *fragment) {
    lv_obj_t **d = fragment->digit_btns;
    for (int i = 0; i < 5; i++) {
        if (i > 0) lv_obj_set_dir_focus_obj(d[i], LV_DIR_LEFT, d[i - 1]);
        if (i < 4) lv_obj_set_dir_focus_obj(d[i], LV_DIR_RIGHT, d[i + 1]);
        lv_obj_set_dir_focus_obj(d[i], LV_DIR_BOTTOM, d[i + 5]);
    }
    for (int i = 5; i < 10; i++) {
        if (i > 5) lv_obj_set_dir_focus_obj(d[i], LV_DIR_LEFT, d[i - 1]);
        if (i < 9) lv_obj_set_dir_focus_obj(d[i], LV_DIR_RIGHT, d[i + 1]);
        lv_obj_set_dir_focus_obj(d[i], LV_DIR_TOP, d[i - 5]);
        lv_obj_set_dir_focus_obj(d[i], LV_DIR_BOTTOM, fragment->delete_btn);
    }
    lv_obj_set_dir_focus_obj(fragment->delete_btn, LV_DIR_TOP, d[7]);
    lv_obj_set_dir_focus_obj(fragment->delete_btn, LV_DIR_RIGHT, fragment->submit_btn);
    lv_obj_set_dir_focus_obj(fragment->submit_btn, LV_DIR_TOP, d[7]);
    lv_obj_set_dir_focus_obj(fragment->submit_btn, LV_DIR_LEFT, fragment->delete_btn);
    lv_obj_set_dir_focus_obj(fragment->submit_btn, LV_DIR_RIGHT, fragment->cancel_btn);
    lv_obj_set_dir_focus_obj(fragment->cancel_btn, LV_DIR_TOP, d[9]);
    lv_obj_set_dir_focus_obj(fragment->cancel_btn, LV_DIR_LEFT, fragment->submit_btn);
}

static void keypad_key(lv_event_t *e) {
    lv_obj_focus_dir_by_key(lv_event_get_target(e), lv_event_get_key(e));
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    stream_pin_fragment_t *fragment = (stream_pin_fragment_t *) self;
    lv_fragment_t *parent = lv_fragment_get_parent(self);
    connection_fragment_set_title(parent, "PIN для стрима");
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    lv_group_focus_obj(fragment->digit_btns[0]);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    stream_pin_fragment_t *fragment = (stream_pin_fragment_t *) self;
    if (fragment->group != NULL) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static void refresh_pin_label(stream_pin_fragment_t *fragment) {
    char display[8];
    memset(display, '_', 4);
    display[4] = '\0';
    for (int i = 0; i < fragment->cursor && i < 4; i++) {
        display[i] = fragment->pin[i];
    }
    lv_label_set_text(fragment->pin_label, display);
}

static void digit_clicked(lv_event_t *e) {
    stream_pin_fragment_t *fragment = lv_event_get_user_data(e);
    if (fragment->cursor >= 4) {
        return;
    }
    char digit = (char) (intptr_t) lv_obj_get_user_data(lv_event_get_current_target(e));
    fragment->pin[fragment->cursor++] = digit;
    fragment->pin[fragment->cursor] = '\0';
    refresh_pin_label(fragment);
}

static void backspace_clicked(lv_event_t *e) {
    stream_pin_fragment_t *fragment = lv_event_get_user_data(e);
    if (fragment->cursor <= 0) {
        return;
    }
    fragment->pin[--fragment->cursor] = '\0';
    refresh_pin_label(fragment);
}

static void submit_clicked(lv_event_t *e) {
    stream_pin_fragment_t *fragment = lv_event_get_user_data(e);
    if (fragment->cursor < 4) {
        return;
    }
    lv_fragment_t *parent = lv_fragment_get_parent((lv_fragment_t *) fragment);
    connection_fragment_submit_stream_pin(parent, fragment->pin);
}

static void cancel_clicked(lv_event_t *e) {
    stream_pin_fragment_t *fragment = lv_event_get_user_data(e);
    lv_fragment_t *parent = lv_fragment_get_parent((lv_fragment_t *) fragment);
    connection_fragment_cancel(parent);
}
