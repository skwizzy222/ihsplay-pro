#include "add_host_fragment.h"

#include "app.h"
#include "backend/host_manager.h"
#include "ui/app_ui.h"
#include "lvgl/theme.h"

#include <string.h>
#include <stdio.h>

typedef struct add_host_fragment_t {
    lv_fragment_t base;
    app_t *app;
    char ip[32];
    lv_obj_t *ip_label;
    lv_obj_t *status_label;
    lv_group_t *group;
} add_host_fragment_t;

static void ctor(lv_fragment_t *self, void *arg);

static void dtor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static bool event_cb(lv_fragment_t *self, int code, void *data);

static void refresh_ip_label(add_host_fragment_t *fragment);

static void digit_clicked(lv_event_t *e);

static void dot_clicked(lv_event_t *e);

static void backspace_clicked(lv_event_t *e);

static void connect_clicked(lv_event_t *e);

const lv_fragment_class_t add_host_fragment_class = {
        .constructor_cb = ctor,
        .destructor_cb = dtor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .event_cb = event_cb,
        .instance_size = sizeof(add_host_fragment_t)
};

static void ctor(lv_fragment_t *self, void *arg) {
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    app_ui_fragment_args_t *args = arg;
    fragment->app = args->app;
    memset(fragment->ip, 0, sizeof(fragment->ip));
}

static void dtor(lv_fragment_t *self) {
    (void) self;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    lv_obj_t *win = app_lv_win_create(container);
    lv_win_add_title(win, "Add Computer by IP");
    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(content, LV_DPX(14), 0);

    lv_obj_t *hint = lv_label_create(content);
    lv_label_set_text(hint, "Enter the local IP of your Steam PC");
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    fragment->ip_label = lv_label_create(content);
    lv_obj_set_style_text_font(fragment->ip_label, fragment->app->ui->font.heading1, 0);
    refresh_ip_label(fragment);

    fragment->status_label = lv_label_create(content);
    lv_label_set_text(fragment->status_label, " ");
    lv_obj_set_style_text_opa(fragment->status_label, LV_OPA_80, 0);

    lv_obj_t *pad = lv_obj_create(content);
    lv_obj_set_size(pad, LV_DPX(420), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_gap(pad, LV_DPX(8), 0);
    lv_obj_set_style_bg_opa(pad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pad, 0, 0);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(pad, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, true);

    static const char *digits[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", NULL};
    for (int i = 0; digits[i] != NULL; i++) {
        lv_obj_t *btn = lv_btn_create(pad);
        lv_obj_set_size(btn, LV_DPX(72), LV_DPX(56));
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, digits[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, digit_clicked, LV_EVENT_CLICKED, fragment);
        lv_obj_set_user_data(btn, (void *) (intptr_t) (digits[i][0]));
        lv_group_add_obj(fragment->group, btn);
    }

    lv_obj_t *dot = lv_btn_create(pad);
    lv_obj_set_size(dot, LV_DPX(72), LV_DPX(56));
    lv_obj_t *dot_label = lv_label_create(dot);
    lv_label_set_text(dot_label, ".");
    lv_obj_center(dot_label);
    lv_obj_add_event_cb(dot, dot_clicked, LV_EVENT_CLICKED, fragment);
    lv_group_add_obj(fragment->group, dot);

    lv_obj_t *actions = lv_obj_create(content);
    lv_obj_set_size(actions, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_gap(actions, LV_DPX(12), 0);
    lv_obj_set_style_bg_opa(actions, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(actions, 0, 0);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);

    lv_obj_t *backspace = lv_btn_create(actions);
    lv_obj_t *backspace_label = lv_label_create(backspace);
    lv_label_set_text(backspace_label, "Delete");
    lv_obj_center(backspace_label);
    lv_obj_add_event_cb(backspace, backspace_clicked, LV_EVENT_CLICKED, fragment);
    lv_group_add_obj(fragment->group, backspace);

    lv_obj_t *connect = lv_btn_create(actions);
    lv_obj_t *connect_label = lv_label_create(connect);
    lv_label_set_text(connect_label, "Find PC");
    lv_obj_center(connect_label);
    lv_obj_add_event_cb(connect, connect_clicked, LV_EVENT_CLICKED, fragment);
    lv_group_add_obj(fragment->group, connect);

    return win;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(indev, fragment->group);
            break;
        }
        indev = lv_indev_get_next(indev);
    }
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    if (fragment->group != NULL) {
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static bool event_cb(lv_fragment_t *self, int code, void *data) {
    (void) data;
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    if (code == APP_UI_NAV_BACK) {
        app_ui_pop_top_fragment(fragment->app->ui);
        return true;
    }
    return false;
}

static void refresh_ip_label(add_host_fragment_t *fragment) {
    if (fragment->ip[0] == '\0') {
        lv_label_set_text(fragment->ip_label, "_.__.__.__");
    } else {
        lv_label_set_text(fragment->ip_label, fragment->ip);
    }
}

static void append_char(add_host_fragment_t *fragment, char ch) {
    size_t len = strlen(fragment->ip);
    if (len + 1 >= sizeof(fragment->ip)) {
        return;
    }
    fragment->ip[len] = ch;
    fragment->ip[len + 1] = '\0';
    refresh_ip_label(fragment);
    lv_label_set_text(fragment->status_label, " ");
}

static void digit_clicked(lv_event_t *e) {
    add_host_fragment_t *fragment = lv_event_get_user_data(e);
    char digit = (char) (intptr_t) lv_obj_get_user_data(lv_event_get_current_target(e));
    append_char(fragment, digit);
}

static void dot_clicked(lv_event_t *e) {
    add_host_fragment_t *fragment = lv_event_get_user_data(e);
    append_char(fragment, '.');
}

static void backspace_clicked(lv_event_t *e) {
    add_host_fragment_t *fragment = lv_event_get_user_data(e);
    size_t len = strlen(fragment->ip);
    if (len == 0) {
        return;
    }
    fragment->ip[len - 1] = '\0';
    refresh_ip_label(fragment);
}

static void connect_clicked(lv_event_t *e) {
    add_host_fragment_t *fragment = lv_event_get_user_data(e);
    if (!host_manager_discover_at(fragment->app->host_manager, fragment->ip)) {
        lv_label_set_text(fragment->status_label, "Invalid IP address");
        return;
    }
    lv_label_set_text(fragment->status_label, "Looking for Steam PC…");
    app_ui_pop_top_fragment(fragment->app->ui);
}
