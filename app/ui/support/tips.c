#include "tips.h"

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *parent);

const lv_fragment_class_t tips_fragment_class = {
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
    lv_label_set_text(title, "Советы");
    lv_obj_set_style_text_color(title, lv_color_hex(0x66c0f4), 0);

    lv_obj_t *body = lv_label_create(content);
    lv_obj_set_width(body, LV_PCT(100));
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body,
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
                      "• D-pad или левый стик джойстика тоже листают меню");
    return content;
}
