#include <assert.h>
#include "app.h"
#include "app_ui.h"
#include "config.h"

#include "launcher.h"

#include "hosts/hosts_fragment.h"
#include "hosts/add_host_fragment.h"
#include "settings/settings.h"
#include "support/support.h"

#include "lvgl/fonts/bootstrap-icons/symbols.h"


#include "backend/host_manager.h"
#include "array_list.h"
#include "lvgl/ext/lv_dir_focus.h"
#include "lvgl/theme.h"
#include "ui/connection/connection_fragment.h"
#include "backend/input_manager.h"

typedef struct launcher_fragment {
    lv_fragment_t base;
    app_t *app;
    lv_coord_t row_dsc[6], col_dsc[4];
    struct {
        lv_style_t root;
        lv_style_t option_icon;
        lv_style_t play_btn;
        lv_style_t play_btn_focused;
        lv_style_t play_btn_pressed;
        lv_style_t option_btn;
        lv_style_t subtitle;
    } styles;
    lv_obj_t *nav_content;
    lv_obj_t *selected_host;
    lv_obj_t *add_host;
    lv_obj_t *gamepads;

    uint64_t selected_host_id;

    int num_launch_options;
} launcher_fragment;

static void constructor(lv_fragment_t *self, void *arg);

static void destructor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static void obj_deleted(lv_fragment_t *self, lv_obj_t *obj);

static void hosts_changed(array_list_t *list, host_manager_hosts_change change_type, int change_index, void *context);

static void launcher_gamepads_changed(launcher_fragment *fragment);;

static bool event_cb(lv_fragment_t *self, int type, void *data);

static lv_obj_t *launch_option_create_label_action(launcher_fragment *fragment, const char *icon, const char *label);

static lv_obj_t *launch_option_get_label(lv_obj_t *obj);

static void launch_option_set_text(lv_obj_t *obj, const char *label);

static void focus_content(lv_event_t *e);

static void open_settings(lv_event_t *e);

static void open_support(lv_event_t *e);

static void select_host(lv_event_t *e);

static void add_host_clicked(lv_event_t *e);

static void request_session(lv_event_t *e);

static void launcher_quit(lv_event_t *e);

static void hosts_update(launcher_fragment *fragment);

static const IHS_HostInfo *get_selected_host(launcher_fragment *fragment);

const lv_fragment_class_t launcher_fragment_class = {
        .constructor_cb = constructor,
        .destructor_cb = destructor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .obj_deleted_cb = obj_deleted,
        .event_cb = event_cb,
        .instance_size = sizeof(launcher_fragment),
};

static const host_manager_listener_t host_manager_listener = {
        .hosts_changed = hosts_changed,
};

static void constructor(lv_fragment_t *self, void *arg) {
    app_ui_fragment_args_t *fargs = arg;
    launcher_fragment *fragment = (launcher_fragment *) self;
    fragment->app = fargs->app;
    fragment->col_dsc[0] = LV_DPX(250);
    fragment->col_dsc[1] = LV_DPX(350);
    fragment->col_dsc[2] = LV_GRID_TEMPLATE_LAST;
    fragment->row_dsc[0] = LV_DPX(40);
    fragment->row_dsc[1] = LV_DPX(40);
    fragment->row_dsc[2] = LV_DPX(40);
    fragment->row_dsc[3] = LV_DPX(40);
    fragment->row_dsc[4] = LV_GRID_FR(1);
    fragment->row_dsc[5] = LV_GRID_TEMPLATE_LAST;

    lv_style_init(&fragment->styles.root);
    lv_style_set_pad_gap(&fragment->styles.root, LV_DPX(10));
    lv_style_set_pad_hor(&fragment->styles.root, 0);
    lv_style_set_pad_top(&fragment->styles.root, LV_DPX(40));
    lv_style_set_pad_bottom(&fragment->styles.root, 0);

    lv_style_init(&fragment->styles.option_icon);
    lv_style_set_text_font(&fragment->styles.option_icon, fragment->app->ui->font.heading3);
    lv_style_set_text_color(&fragment->styles.option_icon, lv_color_hex(0x66c0f4));
    lv_style_set_translate_y(&fragment->styles.option_icon, LV_DPX(4));

    lv_style_init(&fragment->styles.subtitle);
    lv_style_set_text_color(&fragment->styles.subtitle, lv_color_hex(0x8f98a0));
    lv_style_set_text_font(&fragment->styles.subtitle, fragment->app->ui->font.small);

    lv_style_init(&fragment->styles.play_btn);
    lv_style_set_bg_color(&fragment->styles.play_btn, lv_color_hex(0x5c7e10));
    lv_style_set_bg_opa(&fragment->styles.play_btn, LV_OPA_COVER);
    lv_style_set_border_width(&fragment->styles.play_btn, 0);
    lv_style_set_radius(&fragment->styles.play_btn, LV_DPX(3));
    lv_style_set_shadow_width(&fragment->styles.play_btn, LV_DPX(20));
    lv_style_set_shadow_color(&fragment->styles.play_btn, lv_color_hex(0x5c7e10));
    lv_style_set_shadow_opa(&fragment->styles.play_btn, LV_OPA_30);
    lv_style_set_text_color(&fragment->styles.play_btn, lv_color_white());

    lv_style_init(&fragment->styles.play_btn_focused);
    lv_style_set_bg_color(&fragment->styles.play_btn_focused, lv_color_hex(0x75b022));
    lv_style_set_bg_opa(&fragment->styles.play_btn_focused, LV_OPA_COVER);
    lv_style_set_shadow_width(&fragment->styles.play_btn_focused, LV_DPX(28));
    lv_style_set_shadow_opa(&fragment->styles.play_btn_focused, LV_OPA_50);
    lv_style_set_outline_width(&fragment->styles.play_btn_focused, LV_DPX(2));
    lv_style_set_outline_color(&fragment->styles.play_btn_focused, lv_color_hex(0xbeee11));
    lv_style_set_outline_opa(&fragment->styles.play_btn_focused, LV_OPA_COVER);
    lv_style_set_outline_pad(&fragment->styles.play_btn_focused, LV_DPX(3));

    lv_style_init(&fragment->styles.play_btn_pressed);
    lv_style_set_bg_color(&fragment->styles.play_btn_pressed, lv_color_hex(0x4c6b22));
    lv_style_set_bg_opa(&fragment->styles.play_btn_pressed, LV_OPA_COVER);

    lv_style_init(&fragment->styles.option_btn);
    lv_style_set_bg_color(&fragment->styles.option_btn, lv_color_hex(0x2a475e));
    lv_style_set_bg_opa(&fragment->styles.option_btn, LV_OPA_COVER);
    lv_style_set_radius(&fragment->styles.option_btn, LV_DPX(3));
    lv_style_set_border_width(&fragment->styles.option_btn, LV_DPX(1));
    lv_style_set_border_color(&fragment->styles.option_btn, lv_color_hex(0x3d6a8a));
    lv_style_set_border_opa(&fragment->styles.option_btn, LV_OPA_50);
    lv_style_set_pad_hor(&fragment->styles.option_btn, LV_DPX(16));
}

static void destructor(lv_fragment_t *self) {
    launcher_fragment *fragment = (launcher_fragment *) self;
    lv_style_reset(&fragment->styles.option_btn);
    lv_style_reset(&fragment->styles.play_btn_pressed);
    lv_style_reset(&fragment->styles.play_btn_focused);
    lv_style_reset(&fragment->styles.play_btn);
    lv_style_reset(&fragment->styles.subtitle);
    lv_style_reset(&fragment->styles.option_icon);
    lv_style_reset(&fragment->styles.root);
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    launcher_fragment *fragment = (launcher_fragment *) self;
    fragment->num_launch_options = 0;
    lv_obj_t *win = app_lv_win_create(container);

    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clean(header);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *brand_icon = lv_label_create(header);
    lv_obj_set_style_text_font(brand_icon, fragment->app->ui->iconfont.heading2, 0);
    lv_obj_set_style_text_color(brand_icon, lv_color_hex(0x66c0f4), 0);
    lv_label_set_text_static(brand_icon, BS_SYMBOL_STEAM);

    lv_obj_t *brand_title = lv_label_create(header);
    lv_obj_set_style_text_font(brand_title, fragment->app->ui->font.heading2, 0);
    lv_obj_set_style_text_color(brand_title, lv_color_white(), 0);
    lv_obj_set_style_pad_left(brand_title, LV_DPX(10), 0);
    lv_label_set_text(brand_title, "STEAM");

    lv_obj_t *brand_sub = lv_label_create(header);
    lv_obj_add_style(brand_sub, &fragment->styles.subtitle, 0);
    lv_obj_set_style_pad_left(brand_sub, LV_DPX(8), 0);
    lv_label_set_text(brand_sub, "Удалённая игра");

    lv_obj_t *spacer = lv_obj_create(header);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_height(spacer, 1);

    lv_obj_add_event_cb(header, focus_content, LV_EVENT_KEY, fragment);

    lv_obj_t *btn_settings = lv_btn_create(header);
    lv_obj_set_size(btn_settings, LV_DPX(40), LV_DPX(40));
    lv_obj_t *btn_settings_icon = lv_label_create(btn_settings);
    lv_obj_set_style_text_font(btn_settings_icon, fragment->app->ui->iconfont.heading3, 0);
    lv_label_set_text_static(btn_settings_icon, BS_SYMBOL_GEAR_FILL);
    lv_obj_center(btn_settings_icon);
    lv_obj_add_event_cb(btn_settings, open_settings, LV_EVENT_CLICKED, fragment);
    lv_obj_add_flag(btn_settings, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *btn_support = lv_btn_create(header);
    lv_obj_set_size(btn_support, LV_DPX(40), LV_DPX(40));
    lv_obj_t *btn_support_icon = lv_label_create(btn_support);
    lv_obj_set_style_text_font(btn_support_icon, fragment->app->ui->iconfont.heading3, 0);
    lv_label_set_text_static(btn_support_icon, BS_SYMBOL_QUESTION_CIRCLE_FILL);
    lv_obj_center(btn_support_icon);
    lv_obj_add_event_cb(btn_support, open_support, LV_EVENT_CLICKED, fragment);
    lv_obj_add_flag(btn_support, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *btn_quit = lv_btn_create(header);
    lv_obj_set_size(btn_quit, LV_DPX(40), LV_DPX(40));
    lv_obj_t *btn_quit_icon = lv_label_create(btn_quit);
    lv_obj_set_style_text_font(btn_quit_icon, fragment->app->ui->iconfont.heading3, 0);
    lv_label_set_text_static(btn_quit_icon, BS_SYMBOL_X_LG);
    lv_obj_center(btn_quit_icon);
    lv_obj_add_event_cb(btn_quit, launcher_quit, LV_EVENT_CLICKED, fragment->app);
    lv_obj_add_flag(btn_quit, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *nav_content = lv_win_get_content(win);
    lv_obj_add_event_cb(nav_content, focus_content, LV_EVENT_KEY, fragment);
    fragment->nav_content = nav_content;

    lv_obj_set_layout(nav_content, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(nav_content, fragment->col_dsc, fragment->row_dsc);
    lv_obj_set_style_pad_gap(nav_content, LV_DPX(20), 0);

    lv_obj_t *btn_play = lv_btn_create(nav_content);
    lv_obj_add_flag(btn_play, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(btn_play, &fragment->styles.play_btn, 0);
    lv_obj_add_style(btn_play, &fragment->styles.play_btn_focused, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play, &fragment->styles.play_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn_play, LV_SIZE_CONTENT, LV_DPX(200));
    lv_obj_set_flex_flow(btn_play, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_play, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_grid_cell(btn_play, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 5);

    lv_obj_t *img_play = lv_label_create(btn_play);
    lv_obj_set_style_text_font(img_play, fragment->app->ui->iconfont.heading1, 0);
    lv_obj_set_style_text_color(img_play, lv_color_white(), 0);
    lv_label_set_text_static(img_play, BS_SYMBOL_PLAY_CIRCLE_FILL);
    lv_obj_t *label_play = lv_label_create(btn_play);
    lv_obj_set_style_text_font(label_play, fragment->app->ui->font.heading3, 0);
    lv_obj_set_style_text_color(label_play, lv_color_white(), 0);
    lv_label_set_text(label_play, "ИГРАТЬ");
    lv_obj_t *label_play_sub = lv_label_create(btn_play);
    lv_obj_set_style_text_color(label_play_sub, lv_color_hex(0xd2e885), 0);
    lv_label_set_text(label_play_sub, "Запустить Remote Play");

    lv_obj_add_event_cb(btn_play, request_session, LV_EVENT_CLICKED, fragment);

    lv_obj_t *selected_host = launch_option_create_label_action(fragment, BS_SYMBOL_DISPLAY, NULL);
    fragment->selected_host = selected_host;

    lv_obj_add_event_cb(selected_host, select_host, LV_EVENT_CLICKED, fragment);

    lv_obj_t *add_host = launch_option_create_label_action(fragment, BS_SYMBOL_WINDOW_DESKTOP, "Добавить компьютер…");
    fragment->add_host = add_host;
    lv_obj_add_event_cb(add_host, add_host_clicked, LV_EVENT_CLICKED, fragment);

    lv_obj_t *gamepads = launch_option_create_label_action(fragment, BS_SYMBOL_CONTROLLER, NULL);
    fragment->gamepads = gamepads;

    lv_obj_set_dir_focus_obj(btn_settings, LV_DIR_RIGHT, btn_support);
    lv_obj_set_dir_focus_obj(btn_settings, LV_DIR_BOTTOM, btn_play);
    lv_obj_set_dir_focus_obj(btn_support, LV_DIR_RIGHT, btn_quit);
    lv_obj_set_dir_focus_obj(btn_support, LV_DIR_LEFT, btn_settings);
    lv_obj_set_dir_focus_obj(btn_support, LV_DIR_BOTTOM, btn_play);
    lv_obj_set_dir_focus_obj(btn_quit, LV_DIR_LEFT, btn_support);
    lv_obj_set_dir_focus_obj(btn_quit, LV_DIR_BOTTOM, btn_play);

    lv_obj_set_dir_focus_obj(btn_play, LV_DIR_TOP, btn_settings);
    lv_obj_set_dir_focus_obj(btn_play, LV_DIR_RIGHT, selected_host);

    lv_obj_set_dir_focus_obj(selected_host, LV_DIR_LEFT, btn_play);
    lv_obj_set_dir_focus_obj(selected_host, LV_DIR_TOP, btn_settings);
    lv_obj_set_dir_focus_obj(selected_host, LV_DIR_BOTTOM, add_host);

    lv_obj_set_dir_focus_obj(add_host, LV_DIR_LEFT, btn_play);
    lv_obj_set_dir_focus_obj(add_host, LV_DIR_TOP, selected_host);
    lv_obj_set_dir_focus_obj(add_host, LV_DIR_BOTTOM, gamepads);

    lv_obj_set_dir_focus_obj(gamepads, LV_DIR_LEFT, btn_play);
    lv_obj_set_dir_focus_obj(gamepads, LV_DIR_TOP, add_host);

    return win;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    launcher_fragment *fragment = (launcher_fragment *) self;

    hosts_update(fragment);
    host_manager_t *hosts_manager = fragment->app->host_manager;
    host_manager_register_listener(hosts_manager, &host_manager_listener, fragment);
    host_manager_discovery_start(hosts_manager);

    hosts_update(fragment);
    launcher_gamepads_changed(fragment);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    launcher_fragment *fragment = (launcher_fragment *) self;
    host_manager_discovery_stop(fragment->app->host_manager);
    host_manager_unregister_listener(fragment->app->host_manager, &host_manager_listener);
}

static void obj_deleted(lv_fragment_t *self, lv_obj_t *obj) {
    LV_UNUSED(obj);
}

void launcher_fragment_set_selected_host(lv_fragment_t *self, uint64_t client_id) {
    launcher_fragment *fragment = (launcher_fragment *) self;
    fragment->selected_host_id = client_id;
}

static void hosts_changed(array_list_t *list, host_manager_hosts_change change_type, int change_index, void *context) {
    launcher_fragment *fragment = (launcher_fragment *) context;
    if (array_list_size(list) > 0 && fragment->selected_host_id == 0) {
        const IHS_HostInfo *host = array_list_get(list, 0);
        fragment->selected_host_id = host->clientId;
    }
    hosts_update(fragment);
}

static void launcher_gamepads_changed(launcher_fragment *fragment) {
    input_manager_t *manager = fragment->app->input_manager;
    size_t count = input_manager_sdl_gamepad_count(manager);
    lv_obj_t *label = launch_option_get_label(fragment->gamepads);
    if (count == 0) {
        lv_label_set_text(label, "Геймпад не подключён");
    } else if (count == 1) {
        lv_label_set_text(label, "Подключён 1 геймпад");
    } else {
        lv_label_set_text_fmt(label, "Геймпадов: %u", (unsigned) count);
    }
}

static bool event_cb(lv_fragment_t *self, int type, void *data) {
    (void) data;
    launcher_fragment *fragment = (launcher_fragment *) self;
    switch (type) {
        case APP_UI_GAMEPAD_DEVICE_CHANGED:
            launcher_gamepads_changed(fragment);
            return true;
        case APP_UI_NAV_BACK:
            return true;
        default:
            return false;
    }
}

static lv_obj_t *launch_option_create_label_action(launcher_fragment *fragment, const char *icon, const char *label) {
    int row_pos = fragment->num_launch_options++;
    lv_obj_t *action = lv_btn_create(fragment->nav_content);
    lv_obj_add_flag(action, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(action, &fragment->styles.option_btn, 0);
    lv_obj_set_grid_cell(action, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, row_pos, 1);
    lv_obj_set_size(action, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(action, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *icon_obj = lv_img_create(action);
    lv_obj_add_style(icon_obj, &fragment->styles.option_icon, 0);
    lv_img_set_src(icon_obj, icon);

    lv_obj_t *label_obj = lv_label_create(action);
    lv_obj_set_flex_grow(label_obj, 1);
    if (label != NULL) {
        lv_label_set_text(label_obj, label);
    }

    return action;
}

static lv_obj_t *launch_option_get_label(lv_obj_t *obj) {
    return lv_obj_get_child(obj, 1);
}

static void launch_option_set_text(lv_obj_t *obj, const char *label) {
    lv_label_set_text(launch_option_get_label(obj), label);
}

static void focus_content(lv_event_t *e) {
    lv_obj_t *current_target = lv_event_get_current_target(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (lv_obj_get_parent(target) != current_target) {
        return;
    }
    lv_obj_focus_dir_by_key(target, lv_event_get_key(e));
}

static void open_settings(lv_event_t *e) {
    launcher_fragment *fragment = lv_event_get_user_data(e);
    app_ui_push_fragment(fragment->app->ui, &settings_fragment_class, NULL);
}

static void open_support(lv_event_t *e) {
    launcher_fragment *fragment = lv_event_get_user_data(e);
    app_ui_push_fragment(fragment->app->ui, &support_fragment_class, NULL);
}

static void select_host(lv_event_t *e) {
    launcher_fragment *fragment = lv_event_get_user_data(e);
    app_ui_push_fragment(fragment->app->ui, &hosts_fragment_class, fragment);
}

static void add_host_clicked(lv_event_t *e) {
    launcher_fragment *fragment = lv_event_get_user_data(e);
    app_ui_push_fragment(fragment->app->ui, &add_host_fragment_class, NULL);
}

static void request_session(lv_event_t *e) {
    launcher_fragment *fragment = lv_event_get_user_data(e);
    const IHS_HostInfo *host = get_selected_host(fragment);
    if (host == NULL) {
        return;
    }
    IHS_HostInfo *data = calloc(1, sizeof(IHS_HostInfo));
    *data = *host;
    app_ui_push_fragment(fragment->app->ui, &connection_fragment_class, data);
}

static void launcher_quit(lv_event_t *e) {
    app_quit(lv_event_get_user_data(e));
}

static void hosts_update(launcher_fragment *fragment) {
    const IHS_HostInfo *host = get_selected_host(fragment);
    if (host != NULL) {
        launch_option_set_text(fragment->selected_host, host->hostname);
    } else {
        launch_option_set_text(fragment->selected_host, "Выберите компьютер…");
    }
}

static const IHS_HostInfo *get_selected_host(launcher_fragment *fragment) {
    host_manager_t *manager = fragment->app->host_manager;
    array_list_t *hosts = host_manager_get_hosts(manager);
    const IHS_HostInfo *host = NULL;
    for (int i = 0, j = array_list_size(hosts); i < j; i++) {
        const IHS_HostInfo *info = array_list_get(hosts, i);
        if (fragment->selected_host_id == info->clientId) {
            host = info;
            break;
        }
    }
    return host;
}
