#include "settings.h"

#include "app.h"
#include "ui/launcher.h"
#include "lvgl/theme.h"
#include "basic.h"
#include "ui/app_ui.h"
#include "ui/i18n.h"
#include "ui/common/key_nav.h"
#include "lvgl/ext/lv_dir_focus.h"

typedef struct settings_fragment {
    lv_fragment_t base;
    app_t *app;
    lv_obj_t *content;
    lv_obj_t *close_btn;
    lv_group_t *group;
} settings_fragment;

static void constructor(lv_fragment_t *self, void *arg);

static void destructor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static bool event_cb(lv_fragment_t *self, int code, void *data);

const lv_fragment_class_t settings_fragment_class = {
        .constructor_cb = constructor,
        .destructor_cb = destructor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .event_cb = event_cb,
        .instance_size = sizeof(settings_fragment),
};

static void constructor(lv_fragment_t *self, void *arg) {
    settings_fragment *fragment = (settings_fragment *) self;
    fragment->app = ((app_ui_fragment_args_t *) arg)->app;
}

static void destructor(lv_fragment_t *self) {
    (void) self;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    settings_fragment *fragment = (settings_fragment *) self;
    lv_obj_t *win = app_lv_win_create(container);
    lv_win_add_title(win, APP_TR(fragment->app, "Settings", "Настройки"));
    fragment->close_btn = app_lv_win_add_close_btn(win, fragment->app);
    fragment->content = lv_win_get_content(win);
    return win;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    settings_fragment *fragment = (settings_fragment *) self;
    lv_fragment_t *f = lv_fragment_create(&settings_basic_fragment_class, fragment->app);
    lv_fragment_manager_replace(self->child_manager, f, &fragment->content);

    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, true);
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    if (fragment->close_btn) {
        lv_group_add_obj(fragment->group, fragment->close_btn);
        ui_obj_add_key_nav(fragment->close_btn);
    }
    lv_obj_t *first_dd = NULL;
    if (f->obj) {
        uint32_t n = lv_obj_get_child_cnt(f->obj);
        for (uint32_t i = 0; i < n; i++) {
            lv_obj_t *row = lv_obj_get_child(f->obj, i);
            uint32_t cn = lv_obj_get_child_cnt(row);
            for (uint32_t j = 0; j < cn; j++) {
                lv_obj_t *ch = lv_obj_get_child(row, j);
                if (lv_obj_check_type(ch, &lv_dropdown_class)) {
                    lv_group_add_obj(fragment->group, ch);
                    if (first_dd == NULL) {
                        first_dd = ch;
                    }
                }
            }
        }
    }
    if (fragment->close_btn && first_dd) {
        lv_obj_set_dir_focus_obj(fragment->close_btn, LV_DIR_BOTTOM, first_dd);
        lv_obj_set_dir_focus_obj(first_dd, LV_DIR_TOP, fragment->close_btn);
    }
    if (first_dd) {
        lv_group_focus_obj(first_dd);
    } else if (fragment->close_btn) {
        lv_group_focus_obj(fragment->close_btn);
    }
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    settings_fragment *fragment = (settings_fragment *) self;
    if (fragment->group) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static bool event_cb(lv_fragment_t *self, int code, void *data) {
    (void) data;
    settings_fragment *fragment = (settings_fragment *) self;
    if (code == APP_UI_NAV_BACK) {
        app_ui_pop_top_fragment(fragment->app->ui);
        return true;
    }
    return false;
}
