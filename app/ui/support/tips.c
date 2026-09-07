#include "tips.h"
#include "app.h"
#include "ui/i18n.h"

typedef struct tips_fragment_t {
    lv_fragment_t base;
    app_t *app;
} tips_fragment_t;

static void constructor(lv_fragment_t *self, void *arg);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

const lv_fragment_class_t tips_fragment_class = {
        .constructor_cb = constructor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(tips_fragment_t)
};

static void constructor(lv_fragment_t *self, void *arg) {
    ((tips_fragment_t *) self)->app = arg;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    app_t *app = ((tips_fragment_t *) self)->app;
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, LV_DPX(12), 0);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, APP_TR(app, "Tips", "Советы"));
    lv_obj_set_style_text_color(title, lv_color_hex(0x66c0f4), 0);

    lv_obj_t *body = lv_label_create(content);
    lv_obj_set_width(body, LV_PCT(100));
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body,
                      APP_TR(app,
                             "How to reduce input lag:\n"
                             "• PC and TV on the same network — Ethernet on the PC is best\n"
                             "• Connect the gamepad to the TV (Gamepad menu), not to the phone\n"
                             "• Close heavy apps on the PC before streaming\n"
                             "• In Steam Remote Play, lower resolution/bitrate if lag is high\n\n"
                             "No sound or black screen:\n"
                             "• Open Settings and change audio output / video decoder\n"
                             "• Pick another module and fully restart the app\n"
                             "• On webOS, hardware modules (ndl / smp) usually work better than sdl\n\n"
                             "Remote and menus:\n"
                             "• Arrows + OK; Magic Remote pointer is a separate mode\n"
                             "• D-pad or left stick also navigate menus",
                             "Как уменьшить задержку ввода:\n"
                             "• ПК и ТВ в одной сети — лучше Ethernet на ПК\n"
                             "• Геймпад подключайте к ТВ (меню «Геймпад»), не к телефону и не параллельно к ПК\n"
                             "• Закройте тяжёлые приложения на ПК перед стримом\n"
                             "• В Steam Remote Play снизьте разрешение/битрейт, если лаг большой\n\n"
                             "Если нет звука или чёрный экран:\n"
                             "• Откройте «Настройки» и смените аудиовыход / видеодекодер\n"
                             "• Выберите другой модуль и полностью перезапустите приложение\n"
                             "• На webOS обычно лучше аппаратные модули (ndl / smp), не sdl\n\n"
                             "Пульт и меню:\n"
                             "• Стрелки + OK; указка Magic Remote — отдельный режим\n"
                             "• D-pad или левый стик джойстика тоже листают меню"));
    return content;
}
