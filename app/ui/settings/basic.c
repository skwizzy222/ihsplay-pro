#include "app.h"
#include "basic.h"

#include "ss4s_modules.h"
#include "array_list.h"

#include <stdio.h>
#include <string.h>

typedef struct basic_fragment {
    lv_fragment_t base;
    app_t *app;
} basic_fragment;

static void constructor(lv_fragment_t *self, void *arg) {
    basic_fragment *fragment = (basic_fragment *) self;
    fragment->app = arg;
}

static lv_obj_t *make_row(lv_obj_t *parent, const char *title, const char *value) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(row, LV_DPX(4), 0);
    lv_obj_set_style_pad_ver(row, LV_DPX(10), 0);
    lv_obj_set_style_pad_hor(row, LV_DPX(12), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1b2838), 0);
    lv_obj_set_style_radius(row, LV_DPX(8), 0);

    lv_obj_t *t = lv_label_create(row);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_opa(t, LV_OPA_70, 0);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_label_set_long_mode(v, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(v, LV_PCT(100));
    return row;
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    basic_fragment *fragment = (basic_fragment *) self;
    app_t *app = fragment->app;

    lv_obj_t *list = lv_obj_create(container);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_ver(list, LV_DPX(16), 0);
    lv_obj_set_style_pad_hor(list, LV_DPX(20), 0);
    lv_obj_set_style_pad_row(list, LV_DPX(12), 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    lv_obj_t *intro = lv_label_create(list);
    lv_label_set_text(intro,
                      "IHSplay Pro — улучшенный форк IHSplay для LG webOS.\n"
                      "Ниже текущие модули вывода (подбираются автоматически под ТВ).");
    lv_label_set_long_mode(intro, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(intro, LV_PCT(100));

    const char *audio = app && app->settings && app->settings->audio_driver
                        ? app->settings->audio_driver
                        : "auto";
    const char *video = app && app->settings && app->settings->video_driver
                        ? app->settings->video_driver
                        : "auto";

    char audio_line[96];
    char video_line[96];
    snprintf(audio_line, sizeof(audio_line), "%s", audio);
    snprintf(video_line, sizeof(video_line), "%s", video);
    make_row(list, "Аудиовыход", audio_line);
    make_row(list, "Видеодекодер", video_line);

    lv_obj_t *tips = lv_label_create(list);
    lv_label_set_text(tips,
                      "Как уменьшить задержку ввода:\n"
                      "• ПК и ТВ в одной сети, лучше Ethernet на ПК\n"
                      "• Геймпад подключайте к ТВ (меню «Геймпад»), не к телефону\n"
                      "• Закройте тяжёлые приложения на ПК перед стримом\n"
                      "• В Steam Remote Play на ПК снизьте разрешение/битрейт, если лаг большой");
    lv_label_set_long_mode(tips, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(tips, LV_PCT(100));
    lv_obj_set_style_pad_top(tips, LV_DPX(8), 0);

    lv_obj_t *nav = lv_label_create(list);
    lv_label_set_text(nav, "Навигация: ↑↓ на пульте или D-pad джойстика, OK/A — выбрать, Back/B — назад.");
    lv_label_set_long_mode(nav, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(nav, LV_PCT(100));
    lv_obj_set_style_text_opa(nav, LV_OPA_70, 0);

    return list;
}

const lv_fragment_class_t settings_basic_fragment_class = {
        .constructor_cb = constructor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(basic_fragment),
};
