#include "support.h"
#include "wiki.h"
#include "feedback.h"
#include "app.h"
#include "lvgl/theme.h"
#include "ui/app_ui.h"

typedef struct support_fragment_t {
    lv_fragment_t base;
    lv_coord_t col_dsc[3], row_dsc[4];
    lv_obj_t *win_content;
    lv_obj_t *close_btn;
    lv_group_t *group;
    int num_btns;
    app_t *app;
} support_fragment_t;

static void constructor(lv_fragment_t *self, void *args);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static bool event_cb(lv_fragment_t *self, int code, void *data);

static void show_page(lv_fragment_t *self, const lv_fragment_class_t *cls);

static lv_obj_t *add_btn(support_fragment_t *fragment, lv_obj_t *parent, const char *text,
                         const lv_fragment_class_t *cls);

static void btn_click_cb(lv_event_t *e);

static void btn_key_cb(lv_event_t *e);

const lv_fragment_class_t support_fragment_class = {
        .constructor_cb = constructor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .event_cb = event_cb,
        .instance_size = sizeof(support_fragment_t)
};

static void constructor(lv_fragment_t *self, void *args) {
    support_fragment_t *fragment = (support_fragment_t *) self;
    fragment->app = ((app_ui_fragment_args_t *) args)->app;
    fragment->col_dsc[0] = LV_DPX(220);
    fragment->col_dsc[1] = LV_GRID_FR(1);
    fragment->col_dsc[2] = LV_GRID_TEMPLATE_LAST;
    fragment->row_dsc[0] = LV_DPX(48);
    fragment->row_dsc[1] = LV_DPX(48);
    fragment->row_dsc[2] = LV_GRID_FR(1);
    fragment->row_dsc[3] = LV_GRID_TEMPLATE_LAST;
    fragment->num_btns = 0;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    support_fragment_t *fragment = (support_fragment_t *) self;
    lv_obj_t *win = app_lv_win_create(parent);
    lv_win_add_title(win, "Поддержка");
    fragment->close_btn = app_lv_win_add_close_btn(win, fragment->app);
    fragment->win_content = lv_win_get_content(win);
    lv_obj_set_style_pad_row(fragment->win_content, LV_DPX(15), 0);
    lv_obj_set_style_pad_column(fragment->win_content, LV_DPX(30), 0);
    lv_obj_set_layout(fragment->win_content, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(fragment->win_content, fragment->col_dsc, fragment->row_dsc);
    lv_obj_add_event_cb(fragment->win_content, btn_click_cb, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->win_content, btn_key_cb, LV_EVENT_KEY, fragment);

    add_btn(fragment, fragment->win_content, "Справка", &wiki_fragment_class);
    add_btn(fragment, fragment->win_content, "О системе", &feedback_fragment_class);

    return win;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    support_fragment_t *fragment = (support_fragment_t *) self;
    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, true);
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    if (fragment->close_btn) {
        lv_group_add_obj(fragment->group, fragment->close_btn);
    }
    /* Buttons are children of win_content */
    uint32_t n = lv_obj_get_child_cnt(fragment->win_content);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *ch = lv_obj_get_child(fragment->win_content, i);
        if (lv_obj_check_type(ch, &lv_btn_class)) {
            lv_group_add_obj(fragment->group, ch);
        }
    }
    if (fragment->num_btns > 0) {
        /* Focus first sidebar button (after close may be last in header) */
        for (uint32_t i = 0; i < n; i++) {
            lv_obj_t *ch = lv_obj_get_child(fragment->win_content, i);
            if (lv_obj_check_type(ch, &lv_btn_class)) {
                lv_group_focus_obj(ch);
                break;
            }
        }
    }
    show_page(self, &wiki_fragment_class);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    support_fragment_t *fragment = (support_fragment_t *) self;
    if (fragment->group) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static bool event_cb(lv_fragment_t *self, int code, void *data) {
    (void) data;
    support_fragment_t *fragment = (support_fragment_t *) self;
    if (code == APP_UI_NAV_BACK) {
        app_ui_pop_top_fragment(fragment->app->ui);
        return true;
    }
    return false;
}

static lv_obj_t *add_btn(support_fragment_t *fragment, lv_obj_t *parent, const char *text,
                         const lv_fragment_class_t *cls) {
    LV_ASSERT_NULL(cls);
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, fragment->num_btns++, 1);
    lv_obj_set_user_data(btn, (void *) cls);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    return btn;
}

static void show_page(lv_fragment_t *self, const lv_fragment_class_t *cls) {
    support_fragment_t *fragment = (support_fragment_t *) self;
    lv_fragment_t *page = lv_fragment_create(cls, fragment->app);
    lv_fragment_manager_replace(self->child_manager, page, &fragment->win_content);
    lv_obj_set_grid_cell(page->obj, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 3);
}

static void btn_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (!lv_obj_check_type(target, &lv_btn_class)) {
        return;
    }
    const lv_fragment_class_t *cls = lv_obj_get_user_data(target);
    if (cls == NULL) {
        return;
    }
    lv_fragment_t *self = lv_event_get_user_data(e);
    show_page(self, cls);
}

static void btn_key_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (!lv_obj_check_type(target, &lv_btn_class)) {
        return;
    }
    lv_fragment_t *self = lv_event_get_user_data(e);
    lv_coord_t pos = lv_obj_get_style_grid_cell_row_pos(target, 0);
    lv_group_t *group = lv_obj_get_group(target);
    switch (lv_event_get_key(e)) {
        case LV_KEY_UP: {
            if (pos > 0) {
                lv_group_focus_prev(group);
            }
            break;
        }
        case LV_KEY_DOWN: {
            support_fragment_t *fragment = (support_fragment_t *) self;
            if (pos < fragment->num_btns - 1) {
                lv_group_focus_next(group);
            }
            break;
        }
        case LV_KEY_ENTER: {
            const lv_fragment_class_t *cls = lv_obj_get_user_data(target);
            if (cls) {
                show_page(self, cls);
            }
            break;
        }
    }
}
