#include "wiki.h"

#include "app.h"
#include "ui/i18n.h"

#include <string.h>

typedef struct wiki_fragment_t {
    lv_fragment_t base;
    app_t *app;
} wiki_fragment_t;

static void constructor(lv_fragment_t *self, void *arg);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

const lv_fragment_class_t wiki_fragment_class = {
        .constructor_cb = constructor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(wiki_fragment_t)
};

static void constructor(lv_fragment_t *self, void *arg) {
    ((wiki_fragment_t *) self)->app = arg;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    app_t *app = ((wiki_fragment_t *) self)->app;
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, LV_DPX(12), 0);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, APP_TR(app, "IHSplay Pro — improved IHSplay fork",
                                    "IHSplay Pro — идеальный форк IHSplay"));
    lv_obj_set_style_text_color(title, lv_color_hex(0x66c0f4), 0);

    lv_obj_t *hint = lv_label_create(content);
    lv_obj_set_size(hint, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_label_set_text(hint,
                      APP_TR(app,
                             "Separate app (org.ihsplay.pro), not the original IHSplay.\n\n"
                             "Quick start:\n"
                             "1) PC and TV on the same network\n"
                             "2) Steam on PC: Remote Play enabled\n"
                             "3) Gamepad — Gamepad menu on the TV\n"
                             "4) Lag and decoder tips — Tips tab\n\n"
                             "Repository:\n"
                             "https://github.com/skwizzy222/ihsplay-pro",
                             "Отдельное приложение (org.ihsplay.pro), не путать с оригинальным IHSplay.\n\n"
                             "Быстрый старт:\n"
                             "1) ПК и ТВ — одна сеть\n"
                             "2) Steam на ПК: Remote Play включён\n"
                             "3) Геймпад — в меню «Геймпад» к ТВ\n"
                             "4) Подробные советы по лагу и декодерам — вкладка «Советы»\n\n"
                             "Репозиторий:\n"
                             "https://github.com/skwizzy222/ihsplay-pro"));

    const char *url = "https://github.com/skwizzy222/ihsplay-pro";
    lv_obj_t *qrcode = lv_qrcode_create(content, LV_DPX(180), lv_color_black(), lv_color_white());
    lv_obj_set_style_bg_opa(qrcode, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(qrcode, lv_color_white(), 0);
    lv_obj_set_style_pad_all(qrcode, LV_DPX(6), 0);
    lv_qrcode_update(qrcode, url, (uint32_t) strlen(url));

    return content;
}
