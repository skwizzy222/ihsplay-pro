#include "wiki.h"

#include <string.h>

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

const lv_fragment_class_t wiki_fragment_class = {
        .create_obj_cb = create_obj,
        .instance_size = sizeof(lv_fragment_t)
};

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent) {
    (void) self;
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, LV_DPX(12), 0);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "IHSplay Pro — идеальный форк IHSplay");
    lv_obj_set_style_text_color(title, lv_color_hex(0x66c0f4), 0);

    lv_obj_t *hint = lv_label_create(content);
    lv_obj_set_size(hint, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_label_set_text(hint,
                      "Отдельное приложение (org.ihsplay.pro), не путать с оригинальным IHSplay.\n\n"
                      "Быстрый старт:\n"
                      "1) ПК и ТВ — одна сеть\n"
                      "2) Steam на ПК: Remote Play включён\n"
                      "3) Геймпад — в меню «Геймпад» к ТВ\n"
                      "4) Подробные советы по лагу и декодерам — вкладка «Советы»\n\n"
                      "Репозиторий:\n"
                      "https://github.com/skwizzy222/ihsplay-pro");

    const char *url = "https://github.com/skwizzy222/ihsplay-pro";
    lv_obj_t *qrcode = lv_qrcode_create(content, LV_DPX(180), lv_color_black(), lv_color_white());
    lv_obj_set_style_bg_opa(qrcode, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(qrcode, lv_color_white(), 0);
    lv_obj_set_style_pad_all(qrcode, LV_DPX(6), 0);
    lv_qrcode_update(qrcode, url, (uint32_t) strlen(url));

    return content;
}
