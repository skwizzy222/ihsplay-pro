#include "streaming_overlay.h"
#include "lvgl/fonts/bootstrap-icons/symbols.h"

#include "app.h"
#include "ui/app_ui.h"
#include "backend/stream_manager.h"
#include "session.h"
#include "lvgl/ext/lv_dir_focus.h"

typedef struct streaming_overlay_fragment_t {
    lv_fragment_t base;
    app_t *app;
    lv_group_t *group;
    lv_obj_t *btn_resume;
    lv_obj_t *btn_quit;
} streaming_overlay_fragment_t;

static void constructor_cb(lv_fragment_t *self, void *args);

static void destructor_cb(lv_fragment_t *self);

static lv_obj_t *create_obj_cb(lv_fragment_t *self, lv_obj_t *container);

static void obj_created_cb(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete_cb(lv_fragment_t *self, lv_obj_t *obj);

static void overlay_key(lv_event_t *e);

static void resume_clicked_cb(lv_event_t *e);

static void quit_clicked_cb(lv_event_t *e);

const lv_fragment_class_t streaming_overlay_class = {
        .constructor_cb = constructor_cb,
        .destructor_cb = destructor_cb,
        .create_obj_cb = create_obj_cb,
        .obj_created_cb = obj_created_cb,
        .obj_will_delete_cb = obj_will_delete_cb,
        .instance_size = sizeof(streaming_overlay_fragment_t),
};

static void constructor_cb(lv_fragment_t *self, void *args) {
    streaming_overlay_fragment_t *fragment = (streaming_overlay_fragment_t *) self;
    fragment->app = args;
}

static void destructor_cb(lv_fragment_t *self) {
    (void) self;
}

static lv_obj_t *create_obj_cb(lv_fragment_t *self, lv_obj_t *container) {
    streaming_overlay_fragment_t *fragment = (streaming_overlay_fragment_t *) self;
    lv_fragment_t *session_fragment = lv_fragment_get_parent(self);

    lv_obj_t *root = lv_obj_create(container);
    lv_obj_remove_style_all(root);
    lv_obj_add_style(root, session_fragment_get_overlay_style(session_fragment), 0);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(root, LV_DPX(20), 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_70, 0);

    lv_obj_t *panel = lv_obj_create(root);
    lv_obj_set_size(panel, LV_DPX(420), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(panel, LV_DPX(28), 0);
    lv_obj_set_style_pad_gap(panel, LV_DPX(16), 0);
    lv_obj_set_style_radius(panel, LV_DPX(12), 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1b2838), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "Меню");
    lv_obj_set_style_text_font(title, fragment->app->ui->font.heading2, 0);

    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "Назад = продолжить   |   выберите действие");
    lv_obj_set_style_text_opa(hint, LV_OPA_70, 0);

    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, true);

    fragment->btn_resume = lv_btn_create(panel);
    lv_obj_set_width(fragment->btn_resume, LV_PCT(100));
    lv_obj_set_height(fragment->btn_resume, LV_DPX(56));
    lv_obj_t *resume_label = lv_label_create(fragment->btn_resume);
    lv_label_set_text(resume_label, "Продолжить стрим");
    lv_obj_center(resume_label);
    lv_obj_add_event_cb(fragment->btn_resume, resume_clicked_cb, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->btn_resume, overlay_key, LV_EVENT_KEY, fragment);
    lv_group_add_obj(fragment->group, fragment->btn_resume);

    fragment->btn_quit = lv_btn_create(panel);
    lv_obj_set_width(fragment->btn_quit, LV_PCT(100));
    lv_obj_set_height(fragment->btn_quit, LV_DPX(56));
    lv_obj_set_style_bg_color(fragment->btn_quit, lv_color_hex(0xc23b22), 0);
    lv_obj_t *quit_label = lv_label_create(fragment->btn_quit);
    lv_label_set_text(quit_label, "Отключиться");
    lv_obj_center(quit_label);
    lv_obj_add_event_cb(fragment->btn_quit, quit_clicked_cb, LV_EVENT_CLICKED, fragment);
    lv_obj_add_event_cb(fragment->btn_quit, overlay_key, LV_EVENT_KEY, fragment);
    lv_group_add_obj(fragment->group, fragment->btn_quit);

    lv_obj_set_dir_focus_obj(fragment->btn_resume, LV_DIR_BOTTOM, fragment->btn_quit);
    lv_obj_set_dir_focus_obj(fragment->btn_quit, LV_DIR_TOP, fragment->btn_resume);

    return root;
}

static void obj_created_cb(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    streaming_overlay_fragment_t *fragment = (streaming_overlay_fragment_t *) self;
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    lv_group_focus_obj(fragment->btn_resume);
}

static void obj_will_delete_cb(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    streaming_overlay_fragment_t *fragment = (streaming_overlay_fragment_t *) self;
    if (fragment->group != NULL) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static void overlay_key(lv_event_t *e) {
    streaming_overlay_fragment_t *fragment = lv_event_get_user_data(e);
    lv_key_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) {
        stream_manager_set_overlay_opened(fragment->app->stream_manager, false);
        return;
    }
    lv_obj_focus_dir_by_key(lv_event_get_target(e), key);
}

static void resume_clicked_cb(lv_event_t *e) {
    streaming_overlay_fragment_t *fragment = lv_event_get_user_data(e);
    stream_manager_set_overlay_opened(fragment->app->stream_manager, false);
}

static void quit_clicked_cb(lv_event_t *e) {
    streaming_overlay_fragment_t *fragment = lv_event_get_user_data(e);
    stream_manager_stop_active(fragment->app->stream_manager);
}
