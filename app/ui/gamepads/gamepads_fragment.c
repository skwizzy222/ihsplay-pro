#include "gamepads_fragment.h"

#include "app.h"
#include "backend/bt_gamepad_manager.h"
#include "ui/app_ui.h"
#include "lvgl/theme.h"
#include "lvgl/ext/msgbox_ext.h"
#include "array_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct gamepads_fragment_t {
    lv_fragment_t base;
    app_t *app;
    array_list_t devices;
    lv_obj_t *list;
    lv_obj_t *hint;
    lv_obj_t *scan_btn;
    lv_timer_t *scan_timer;
    bool scanning;
    lv_group_t *group;
    char pending_address[24];
    char pending_name[96];
} gamepads_fragment_t;

static void ctor(lv_fragment_t *self, void *arg);

static void dtor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj);

static bool event_cb(lv_fragment_t *self, int code, void *data);

static void rebuild_list(gamepads_fragment_t *fragment);

static void scan_clicked(lv_event_t *e);

static void device_clicked(lv_event_t *e);

static void device_btn_deleted(lv_event_t *e);

static void list_key_cb(lv_event_t *e);

static void scan_timer_cb(lv_timer_t *timer);

static void show_msg(const char *text);

static void open_device_menu(gamepads_fragment_t *fragment, const bt_gamepad_device_t *dev);

static void device_menu_cb(lv_event_t *e);

const lv_fragment_class_t gamepads_fragment_class = {
        .constructor_cb = ctor,
        .destructor_cb = dtor,
        .create_obj_cb = create_obj,
        .obj_created_cb = obj_created,
        .obj_will_delete_cb = obj_will_delete,
        .event_cb = event_cb,
        .instance_size = sizeof(gamepads_fragment_t)
};

static void ctor(lv_fragment_t *self, void *arg) {
    gamepads_fragment_t *fragment = (gamepads_fragment_t *) self;
    fragment->app = ((app_ui_fragment_args_t *) arg)->app;
    array_list_init(&fragment->devices, sizeof(bt_gamepad_device_t), 16);
    fragment->scanning = false;
    fragment->scan_timer = NULL;
    fragment->pending_address[0] = '\0';
    fragment->pending_name[0] = '\0';
}

static void dtor(lv_fragment_t *self) {
    gamepads_fragment_t *fragment = (gamepads_fragment_t *) self;
    array_list_deinit(&fragment->devices);
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    gamepads_fragment_t *fragment = (gamepads_fragment_t *) self;
    lv_obj_t *win = app_lv_win_create(container);
    lv_win_add_title(win, "Геймпады");
    app_lv_win_add_close_btn(win, fragment->app);

    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(content, LV_DPX(16), 0);
    lv_obj_set_style_pad_row(content, LV_DPX(12), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    fragment->hint = lv_label_create(content);
    if (bt_gamepad_available()) {
        lv_label_set_text(fragment->hint,
                          "1) Искать  2) Sync/pairing на геймпаде  3) Выбрать → Подключить.\n"
                          "Пока идёт сопряжение не выходите из меню (до ~30 сек). Листайте ↑↓.");
    } else {
        lv_label_set_text(fragment->hint, "Bluetooth-геймпады доступны только на webOS TV.");
    }
    lv_label_set_long_mode(fragment->hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(fragment->hint, LV_PCT(100));

    fragment->scan_btn = lv_btn_create(content);
    lv_obj_set_width(fragment->scan_btn, LV_PCT(100));
    lv_obj_set_height(fragment->scan_btn, LV_DPX(48));
    lv_obj_t *scan_label = lv_label_create(fragment->scan_btn);
    lv_label_set_text(scan_label, bt_gamepad_available() ? "Искать геймпады" : "Недоступно");
    lv_obj_center(scan_label);
    lv_obj_add_event_cb(fragment->scan_btn, scan_clicked, LV_EVENT_CLICKED, fragment);
    if (!bt_gamepad_available()) {
        lv_obj_add_state(fragment->scan_btn, LV_STATE_DISABLED);
    }

    fragment->list = lv_list_create(content);
    lv_obj_set_width(fragment->list, LV_PCT(100));
    lv_obj_set_flex_grow(fragment->list, 1);
    lv_obj_set_scroll_dir(fragment->list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(fragment->list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_flag(fragment->list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(fragment->list, list_key_cb, LV_EVENT_KEY, fragment);

    return win;
}

static void obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    gamepads_fragment_t *fragment = (gamepads_fragment_t *) self;
    fragment->group = lv_group_create();
    lv_group_set_wrap(fragment->group, false);
    app_ui_push_modal_group(fragment->app->ui, fragment->group);
    lv_group_add_obj(fragment->group, fragment->scan_btn);
    lv_group_add_obj(fragment->group, fragment->list);

    if (bt_gamepad_available()) {
        bt_gamepad_refresh_devices(&fragment->devices);
        rebuild_list(fragment);
    }
    lv_group_focus_obj(fragment->scan_btn);
}

static void obj_will_delete(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    gamepads_fragment_t *fragment = (gamepads_fragment_t *) self;
    if (fragment->scan_timer != NULL) {
        lv_timer_del(fragment->scan_timer);
        fragment->scan_timer = NULL;
    }
    if (fragment->scanning) {
        bt_gamepad_stop_scan();
        fragment->scanning = false;
    }
    if (fragment->group != NULL) {
        app_ui_remove_modal_group(fragment->app->ui, fragment->group);
        lv_group_del(fragment->group);
        fragment->group = NULL;
    }
}

static bool event_cb(lv_fragment_t *self, int code, void *data) {
    (void) data;
    if (code == APP_UI_NAV_BACK) {
        app_ui_pop_top_fragment(((gamepads_fragment_t *) self)->app->ui);
        return true;
    }
    return false;
}

static void list_key_cb(lv_event_t *e) {
    lv_obj_t *list = lv_event_get_target(e);
    lv_key_t key = lv_event_get_key(e);
    if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT) {
        lv_obj_scroll_by_bounded(list, 0, -LV_DPX(56), LV_ANIM_ON);
    } else if (key == LV_KEY_UP || key == LV_KEY_LEFT) {
        lv_obj_scroll_by_bounded(list, 0, LV_DPX(56), LV_ANIM_ON);
    }
}

static void rebuild_list(gamepads_fragment_t *fragment) {
    lv_obj_clean(fragment->list);
    if (fragment->group) {
        lv_group_remove_all_objs(fragment->group);
        lv_group_add_obj(fragment->group, fragment->scan_btn);
        lv_group_add_obj(fragment->group, fragment->list);
    }

    for (int i = 0, n = (int) array_list_size(&fragment->devices); i < n; i++) {
        bt_gamepad_device_t *d = array_list_get(&fragment->devices, i);
        const char *status = d->connected_hid ? "HID подключён"
                                              : (d->paired ? "сопряжён" : "найден");
        const char *kind = d->likely_gamepad ? "геймпад?" : "другое BT";
        char line[192];
        snprintf(line, sizeof(line), "%s\n%s · %s · %s", d->name, d->address, status, kind);

        lv_obj_t *btn = lv_list_add_btn(fragment->list, NULL, line);
        lv_obj_set_height(btn, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(btn, LV_DPX(10), 0);
        /* Make label wrap */
        uint32_t child_cnt = lv_obj_get_child_cnt(btn);
        for (uint32_t c = 0; c < child_cnt; c++) {
            lv_obj_t *ch = lv_obj_get_child(btn, c);
            if (lv_obj_check_type(ch, &lv_label_class)) {
                lv_label_set_long_mode(ch, LV_LABEL_LONG_WRAP);
                lv_obj_set_width(ch, LV_PCT(90));
            }
        }

        bt_gamepad_device_t *copy = malloc(sizeof(*copy));
        if (copy) {
            *copy = *d;
        }
        lv_obj_set_user_data(btn, copy);
        lv_obj_add_event_cb(btn, device_clicked, LV_EVENT_CLICKED, fragment);
        lv_obj_add_event_cb(btn, device_btn_deleted, LV_EVENT_DELETE, NULL);
        if (fragment->group) {
            lv_group_add_obj(fragment->group, btn);
        }
    }

    if (array_list_size(&fragment->devices) == 0) {
        lv_list_add_text(fragment->list,
                         fragment->scanning
                         ? "Поиск геймпадов… (анонимный BLE скрыт)"
                         : "Подходящих устройств нет. Запустите поиск и sync на джойстике.");
    }
}

static void device_btn_deleted(lv_event_t *e) {
    free(lv_obj_get_user_data(lv_event_get_target(e)));
}

static void scan_clicked(lv_event_t *e) {
    gamepads_fragment_t *fragment = lv_event_get_user_data(e);
    if (!bt_gamepad_available()) {
        show_msg("Bluetooth доступен только на webOS TV");
        return;
    }
    if (!fragment->scanning) {
        if (!bt_gamepad_start_scan()) {
            show_msg("Не удалось начать поиск Bluetooth");
            return;
        }
        fragment->scanning = true;
        lv_label_set_text(lv_obj_get_child(fragment->scan_btn, 0), "Остановить поиск");
        lv_label_set_text(fragment->hint, "Поиск… Нажмите sync на джойстике. Листайте список ↓↑.");
        if (fragment->scan_timer == NULL) {
            fragment->scan_timer = lv_timer_create(scan_timer_cb, 1500, fragment);
        }
    } else {
        bt_gamepad_stop_scan();
        fragment->scanning = false;
        lv_label_set_text(lv_obj_get_child(fragment->scan_btn, 0), "Искать геймпады");
        lv_label_set_text(fragment->hint,
                          "Искать → sync на джойстике → выбрать → Подключить / Отключить / Забыть.");
        if (fragment->scan_timer != NULL) {
            lv_timer_del(fragment->scan_timer);
            fragment->scan_timer = NULL;
        }
    }
    bt_gamepad_refresh_devices(&fragment->devices);
    rebuild_list(fragment);
}

static void scan_timer_cb(lv_timer_t *timer) {
    gamepads_fragment_t *fragment = timer->user_data;
    bt_gamepad_refresh_devices(&fragment->devices);
    rebuild_list(fragment);
}

static void device_clicked(lv_event_t *e) {
    gamepads_fragment_t *fragment = lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const bt_gamepad_device_t *dev = lv_obj_get_user_data(btn);
    if (dev == NULL || dev->address[0] == '\0') {
        return;
    }
    open_device_menu(fragment, dev);
}

static void open_device_menu(gamepads_fragment_t *fragment, const bt_gamepad_device_t *dev) {
    snprintf(fragment->pending_address, sizeof(fragment->pending_address), "%s", dev->address);
    snprintf(fragment->pending_name, sizeof(fragment->pending_name), "%s", dev->name);

    char title[128];
    snprintf(title, sizeof(title), "%s", dev->name);
    char body[192];
    snprintf(body, sizeof(body), "%s\n%s",
             dev->address,
             dev->likely_gamepad ? "Похоже на геймпад" : "Скорее не геймпад — connect может не сработать");

    static const char *btns[] = {"Подключить", "Отключить", "Забыть", "Отмена", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, title, body, btns, false);
    lv_obj_add_event_cb(mbox, device_menu_cb, LV_EVENT_VALUE_CHANGED, fragment);
    lv_obj_center(mbox);
}

static void device_menu_cb(lv_event_t *e) {
    gamepads_fragment_t *fragment = lv_event_get_user_data(e);
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint16_t idx = lv_msgbox_get_active_btn(mbox);
    char err[160] = {0};
    const char *addr = fragment->pending_address;

    if (idx == 0) {
        /* Connect */
        lv_msgbox_close_async(mbox);
        lv_label_set_text(fragment->hint, "Подключение HID…");
        lv_refr_now(NULL);
        bool ok = bt_gamepad_connect(addr, err, sizeof(err));
        bt_gamepad_refresh_devices(&fragment->devices);
        rebuild_list(fragment);
        if (ok) {
            show_msg("HID-геймпад подключён.");
            lv_label_set_text(fragment->hint, "Геймпад подключён. Можно к лаунчеру.");
        } else {
            show_msg(err[0] ? err : "Не удалось подключить");
            lv_label_set_text(fragment->hint, "Подключение не удалось.");
        }
    } else if (idx == 1) {
        lv_msgbox_close_async(mbox);
        bool ok = bt_gamepad_disconnect(addr, err, sizeof(err));
        bt_gamepad_refresh_devices(&fragment->devices);
        rebuild_list(fragment);
        show_msg(ok ? "Отключено." : (err[0] ? err : "Не удалось отключить"));
    } else if (idx == 2) {
        lv_msgbox_close_async(mbox);
        bool ok = bt_gamepad_unpair(addr, err, sizeof(err));
        bt_gamepad_refresh_devices(&fragment->devices);
        rebuild_list(fragment);
        show_msg(ok ? "Устройство забыто." : (err[0] ? err : "Не удалось забыть"));
    } else {
        lv_msgbox_close_async(mbox);
    }
}

static void msgbox_close_cb(lv_event_t *e) {
    lv_msgbox_close_async(lv_event_get_current_target(e));
}

static void show_msg(const char *text) {
    static const char *btns[] = {"OK", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, NULL, text, btns, false);
    lv_obj_add_event_cb(mbox, msgbox_close_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
}
