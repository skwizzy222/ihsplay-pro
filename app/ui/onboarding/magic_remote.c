#include "magic_remote.h"

#include "app.h"
#include "settings/app_settings.h"
#include "ui/app_ui.h"
#include "ui/i18n.h"
#include "ui/common/key_nav.h"
#include "lvgl/ext/lv_dir_focus.h"
#include "lvgl/theme.h"

typedef struct magic_remote_onboarding_fragment_t {
    lv_fragment_t base;
    app_t *app;
    lv_group_t *group;
    lv_obj_t *step_label;
    lv_obj_t *title_label;
    lv_obj_t *body_label;
    lv_obj_t *btn_skip;
    lv_obj_t *btn_next;
    int step;
    bool closing;
} magic_remote_onboarding_fragment_t;

enum {
    ONBOARD_STEPS = 3
};

static void constructor(lv_fragment_t *self, void *args);

static void destructor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static bool event_cb(lv_fragment_t *self, int code, void *data);

static void refresh_step(magic_remote_onboarding_fragment_t *fragment);

static void finish_onboarding(magic_remote_onboarding_fragment_t *fragment);

static void skip_clicked(lv_event_t *e);

static void next_clicked(lv_event_t *e);

const lv_fragment_class_t magic_remote_onboarding_fragment_class = {
        .constructor_cb = constructor,
        .destructor_cb = destructor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .event_cb = event_cb,
        .instance_size = sizeof(magic_remote_onboarding_fragment_t)
};

static void constructor(lv_fragment_t *self, void *args) {
    magic_remote_onboarding_fragment_t *fragment = (magic_remote_onboarding_fragment_t *) self;
    fragment->app = ((app_ui_fragment_args_t *) args)->app;
    fragment->step = 0;
    fragment->closing = false;
}

static void destructor(lv_fragment_t *self) {
    (void) self;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    magic_remote_onboarding_fragment_t *fragment = (magic_remote_onboarding_fragment_t *) self;
    app_t *app = fragment->app;

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0e141b), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(root, LV_DPX(40), 0);
    lv_obj_set_style_pad_row(root, LV_DPX(20), 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(root);
    lv_obj_set_size(card, LV_DPX(720), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card, LV_DPX(36), 0);
    lv_obj_set_style_pad_row(card, LV_DPX(18), 0);
    lv_obj_set_style_radius(card, LV_DPX(14), 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1b2838), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    fragment->step_label = lv_label_create(card);
    lv_obj_set_style_text_color(fragment->step_label, lv_color_hex(0x66c0f4), 0);
    lv_obj_set_style_text_opa(fragment->step_label, LV_OPA_80, 0);

    fragment->title_label = lv_label_create(card);
    lv_obj_set_style_text_font(fragment->title_label, app->ui->font.heading2, 0);
    lv_obj_set_width(fragment->title_label, LV_PCT(100));
    lv_label_set_long_mode(fragment->title_label, LV_LABEL_LONG_WRAP);

    fragment->body_label = lv_label_create(card);
    lv_obj_set_width(fragment->body_label, LV_PCT(100));
    lv_label_set_long_mode(fragment->body_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_opa(fragment->body_label, LV_OPA_90, 0);

    lv_obj_t *actions = lv_obj_create(card);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, LV_DPX(16), 0);
    lv_obj_set_style_pad_top(actions, LV_DPX(12), 0);

    fragment->btn_skip = lv_btn_create(actions);
    lv_obj_set_size(fragment->btn_skip, LV_DPX(160), LV_DPX(52));
    lv_obj_set_style_bg_opa(fragment->btn_skip, LV_OPA_30, 0);
    lv_obj_t *skip_lbl = lv_label_create(fragment->btn_skip);
    lv_label_set_text(skip_lbl, APP_TR(app, "Skip", "Пропустить"));
    lv_obj_center(skip_lbl);
    lv_obj_add_event_cb(fragment->btn_skip, skip_clicked, LV_EVENT_CLICKED, fragment);
    ui_obj_add_key_nav(fragment->btn_skip);

    fragment->btn_next = lv_btn_create(actions);
    lv_obj_set_size(fragment->btn_next, LV_DPX(200), LV_DPX(52));
    lv_obj_set_style_bg_color(fragment->btn_next, lv_color_hex(0x66c0f4), 0);
    lv_obj_t *next_lbl = lv_label_create(fragment->btn_next);
    lv_label_set_text(next_lbl, APP_TR(app, "Next", "Далее"));
    lv_obj_center(next_lbl);
    lv_obj_add_event_cb(fragment->btn_next, next_clicked, LV_EVENT_CLICKED, fragment);
    ui_obj_add_key_nav(fragment->btn_next);

    lv_obj_set_dir_focus_obj(fragment->btn_skip, LV_DIR_RIGHT, fragment->btn_next);
    lv_obj_set_dir_focus_obj(fragment->btn_next, LV_DIR_LEFT, fragment->btn_skip);

    refresh_step(fragment);
    return root;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    magic_remote_onboarding_fragment_t *fragment = (magic_remote_onboarding_fragment_t *) self;
    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, true);
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    lv_group_add_obj(fragment->group, fragment->btn_skip);
    lv_group_add_obj(fragment->group, fragment->btn_next);
    lv_group_focus_obj(fragment->btn_next);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    magic_remote_onboarding_fragment_t *fragment = (magic_remote_onboarding_fragment_t *) self;
    if (fragment->group) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static bool event_cb(lv_fragment_t *self, int code, void *data) {
    (void) data;
    magic_remote_onboarding_fragment_t *fragment = (magic_remote_onboarding_fragment_t *) self;
    if (code == APP_UI_NAV_BACK) {
        finish_onboarding(fragment);
        return true;
    }
    return false;
}

static void refresh_step(magic_remote_onboarding_fragment_t *fragment) {
    app_t *app = fragment->app;
    lv_label_set_text_fmt(fragment->step_label,
                          APP_TR(app, "Magic Remote · %d / %d", "Magic Remote · %d / %d"),
                          fragment->step + 1, ONBOARD_STEPS);

    lv_obj_t *next_lbl = lv_obj_get_child(fragment->btn_next, 0);
    if (fragment->step >= ONBOARD_STEPS - 1) {
        lv_label_set_text(next_lbl, APP_TR(app, "Got it", "Понятно"));
    } else {
        lv_label_set_text(next_lbl, APP_TR(app, "Next", "Далее"));
    }

    switch (fragment->step) {
        case 0:
            lv_label_set_text(fragment->title_label,
                              APP_TR(app, "Open the stream menu", "Открыть меню стрима"));
            lv_label_set_text(fragment->body_label,
                              APP_TR(app,
                                     "During a stream, press BACK on the Magic Remote "
                                     "(or EXIT). That opens the in-app overlay — it does "
                                     "not quit Steam by itself.\n\n"
                                     "On a gamepad, the Back / Select button does the same.",
                                     "Во время стрима нажмите BACK на Magic Remote "
                                     "(или EXIT). Откроется оверлей приложения — это "
                                     "ещё не выход из Steam.\n\n"
                                     "На геймпаде то же делает кнопка Back / Select."));
            break;
        case 1:
            lv_label_set_text(fragment->title_label,
                              APP_TR(app, "Leave the stream", "Выйти из стрима"));
            lv_label_set_text(fragment->body_label,
                              APP_TR(app,
                                     "In the overlay choose Disconnect to stop Remote Play "
                                     "and return to the app home screen.\n\n"
                                     "Choose Resume — or press BACK again — to keep playing.",
                                     "В оверлее выберите «Отключиться», чтобы остановить "
                                     "Remote Play и вернуться на главный экран приложения.\n\n"
                                     "«Продолжить» или снова BACK — вернуться к игре."));
            break;
        default:
            lv_label_set_text(fragment->title_label,
                              APP_TR(app, "Menus and BACK", "Меню и BACK"));
            lv_label_set_text(fragment->body_label,
                              APP_TR(app,
                                     "In app menus: arrows + OK to move and confirm. "
                                     "BACK goes one screen back.\n\n"
                                     "The Magic Remote pointer is optional — you can use "
                                     "D-pad only. Reopen this guide anytime: Support → Remote.",
                                     "В меню приложения: стрелки + OK. BACK — на экран назад.\n\n"
                                     "Указка Magic Remote необязательна — можно только "
                                     "D-pad. Снова открыть: Поддержка → Пульт."));
            break;
    }
}

static void finish_onboarding_action(app_t *app, void *data) {
    (void) data;
    /* Pop only if onboarding is still on top (avoids double-pop races). */
    if (lv_fragment_manager_get_stack_size(app->ui->fm) < 2) {
        return;
    }
    app_ui_pop_top_fragment(app->ui);
}

static void finish_onboarding(magic_remote_onboarding_fragment_t *fragment) {
    if (fragment->closing) {
        return;
    }
    fragment->closing = true;
    app_settings_set_magic_remote_onboarding_done(fragment->app->settings, true);
    /* Never destroy the fragment from inside LV_EVENT_CLICKED — defer pop. */
    app_run_on_main(fragment->app, finish_onboarding_action, NULL);
}

static void skip_clicked(lv_event_t *e) {
    finish_onboarding(lv_event_get_user_data(e));
}

static void next_clicked(lv_event_t *e) {
    magic_remote_onboarding_fragment_t *fragment = lv_event_get_user_data(e);
    if (fragment->closing) {
        return;
    }
    if (fragment->step >= ONBOARD_STEPS - 1) {
        finish_onboarding(fragment);
        return;
    }
    fragment->step++;
    refresh_step(fragment);
}
