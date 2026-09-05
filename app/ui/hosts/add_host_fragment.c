#include "add_host_fragment.h"

#include "app.h"
#include "backend/host_manager.h"
#include "ui/app_ui.h"
#include "lvgl/theme.h"
#include "lvgl/ext/lv_dir_focus.h"

#include <string.h>
#include <stdio.h>

typedef struct add_host_fragment_t {
    lv_fragment_t base;
    app_t *app;
    char ip[32];
    lv_obj_t *ip_label;
    lv_obj_t *status_label;
    lv_group_t *group;
    lv_obj_t *digit_btns[10];
    lv_obj_t *dot_btn;
    lv_obj_t *delete_btn;
    lv_obj_t *find_btn;
    lv_obj_t *back_btn;
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

static void nav_back_clicked(lv_event_t *e);

static void keypad_key(lv_event_t *e);

static void wire_dir_focus(add_host_fragment_t *fragment);

const lv_fragment_class_t add_host_fragment_class = {
        .constructor_cb = ctor,
        .destructor_cb = dtor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .event_cb = event_cb,
        .instance_size = sizeof(add_host_fragment_t)
};

static lv_obj_t *make_pad_btn(lv_obj_t *parent, lv_group_t *group, const char *text,
                              lv_event_cb_t cb, void *user_data) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_DPX(72), LV_DPX(56));
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_add_event_cb(btn, keypad_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(group, btn);
    return btn;
}

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
    lv_label_set_text(hint, "Введите локальный IP компьютера со Steam");
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

    static const char *digits[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int i = 0; i < 10; i++) {
        fragment->digit_btns[i] = make_pad_btn(pad, fragment->group, digits[i], digit_clicked, fragment);
        lv_obj_set_user_data(fragment->digit_btns[i], (void *) (intptr_t) digits[i][0]);
    }

    fragment->dot_btn = make_pad_btn(pad, fragment->group, ".", dot_clicked, fragment);

    lv_obj_t *actions = lv_obj_create(content);
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

    fragment->find_btn = lv_btn_create(actions);
    lv_obj_t *connect_label = lv_label_create(fragment->find_btn);
    lv_label_set_text(connect_label, "Найти ПК");
    lv_obj_center(connect_label);
    lv_obj_add_event_cb(fragment->find_btn, connect_clicked, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->find_btn, keypad_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(fragment->group, fragment->find_btn);

    fragment->back_btn = lv_btn_create(actions);
    lv_obj_t *back_label = lv_label_create(fragment->back_btn);
    lv_label_set_text(back_label, "Закрыть");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(fragment->back_btn, nav_back_clicked, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->back_btn, keypad_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(fragment->group, fragment->back_btn);

    wire_dir_focus(fragment);
    return win;
}

static void wire_dir_focus(add_host_fragment_t *fragment) {
    lv_obj_t **d = fragment->digit_btns;
    /* Row 1: 1 2 3 4 5 */
    for (int i = 0; i < 5; i++) {
        if (i > 0) {
            lv_obj_set_dir_focus_obj(d[i], LV_DIR_LEFT, d[i - 1]);
        }
        if (i < 4) {
            lv_obj_set_dir_focus_obj(d[i], LV_DIR_RIGHT, d[i + 1]);
        }
        lv_obj_set_dir_focus_obj(d[i], LV_DIR_BOTTOM, d[i + 5]);
    }
    /* Row 2: 6 7 8 9 0 */
    for (int i = 5; i < 10; i++) {
        if (i > 5) {
            lv_obj_set_dir_focus_obj(d[i], LV_DIR_LEFT, d[i - 1]);
        }
        if (i < 9) {
            lv_obj_set_dir_focus_obj(d[i], LV_DIR_RIGHT, d[i + 1]);
        }
        lv_obj_set_dir_focus_obj(d[i], LV_DIR_TOP, d[i - 5]);
        lv_obj_set_dir_focus_obj(d[i], LV_DIR_BOTTOM, fragment->dot_btn);
    }
    /* Dot */
    lv_obj_set_dir_focus_obj(fragment->dot_btn, LV_DIR_TOP, d[7]);
    lv_obj_set_dir_focus_obj(fragment->dot_btn, LV_DIR_BOTTOM, fragment->delete_btn);
    lv_obj_set_dir_focus_obj(fragment->dot_btn, LV_DIR_LEFT, d[5]);
    lv_obj_set_dir_focus_obj(fragment->dot_btn, LV_DIR_RIGHT, d[9]);
    /* Actions */
    lv_obj_set_dir_focus_obj(fragment->delete_btn, LV_DIR_TOP, fragment->dot_btn);
    lv_obj_set_dir_focus_obj(fragment->delete_btn, LV_DIR_RIGHT, fragment->find_btn);
    lv_obj_set_dir_focus_obj(fragment->find_btn, LV_DIR_TOP, fragment->dot_btn);
    lv_obj_set_dir_focus_obj(fragment->find_btn, LV_DIR_LEFT, fragment->delete_btn);
    lv_obj_set_dir_focus_obj(fragment->find_btn, LV_DIR_RIGHT, fragment->back_btn);
    lv_obj_set_dir_focus_obj(fragment->back_btn, LV_DIR_TOP, fragment->dot_btn);
    lv_obj_set_dir_focus_obj(fragment->back_btn, LV_DIR_LEFT, fragment->find_btn);
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    lv_group_focus_obj(fragment->digit_btns[0]);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    add_host_fragment_t *fragment = (add_host_fragment_t *) self;
    if (fragment->group != NULL) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
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

static void keypad_key(lv_event_t *e) {
    lv_key_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) {
        add_host_fragment_t *fragment = NULL;
        /* ESC handled globally via APP_UI_NAV_BACK */
        (void) fragment;
        return;
    }
    lv_obj_focus_dir_by_key(lv_event_get_target(e), key);
}

static void refresh_ip_label(add_host_fragment_t *fragment) {
    if (fragment->ip[0] == '\0') {
        lv_label_set_text(fragment->ip_label, "___.___.___.___");
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
        lv_label_set_text(fragment->status_label, "Неверный IP-адрес");
        return;
    }
    lv_label_set_text(fragment->status_label, "Ищем компьютер со Steam…");
    app_ui_pop_top_fragment(fragment->app->ui);
}

static void nav_back_clicked(lv_event_t *e) {
    add_host_fragment_t *fragment = lv_event_get_user_data(e);
    app_ui_pop_top_fragment(fragment->app->ui);
}
