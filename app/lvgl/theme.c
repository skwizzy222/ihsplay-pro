#include "theme.h"
#include "config.h"

#include "app.h"
#include "ui/app_ui.h"

#include "ext/msgbox_ext.h"
#include "lvgl/fonts/bootstrap-icons/symbols.h"

/* Steam-inspired palette:
 *  #171a21 header / deepest
 *  #1b2838 main surface
 *  #2a475e panels
 *  #66c0f4 accent text
 *  #1a9fff focus CTA
 *  #5c7e10 play green
 *  #c7d5e0 body text
 */

#define STEAM_BG_DEEP      0x171a21
#define STEAM_BG_MAIN      0x1b2838
#define STEAM_BG_PANEL     0x2a475e
#define STEAM_ACCENT       0x66c0f4
#define STEAM_FOCUS        0x1a9fff
#define STEAM_PLAY         0x5c7e10
#define STEAM_PLAY_HOVER   0x75b022
#define STEAM_TEXT         0xc7d5e0
#define STEAM_TEXT_MUTED   0x8f98a0

static void apply_cb(lv_theme_t *theme, lv_obj_t *obj);

typedef struct theme_context_t {
    app_ui_t *ui;

    lv_style_t scr;

    lv_style_t obj;

    lv_style_t focused;

    lv_style_t label;

    lv_style_t btn;
    lv_style_t btn_focused;
    lv_style_t btn_pressed;

    lv_style_t dropdown_list;

    lv_style_t modal_bg;

    lv_style_t msgbox;
    lv_style_t msgbox_backdrop;
    lv_style_t msgbox_title;
    lv_style_t msgbox_text;
    lv_style_t msgbox_btns;
    lv_style_t msgbox_btns_item;
    lv_style_t msgbox_btns_item_focused;
    lv_style_t msgbox_btns_item_pressed;

    lv_style_t win_title;
    lv_style_t win_btn;
    lv_style_t win_header;
    lv_style_t win_content;

    lv_style_t arc_indic;
    lv_style_t arc_indic_primary;

} theme_context_t;

void app_theme_init(lv_theme_t *theme, app_ui_t *ui) {
    theme_context_t *styles = lv_mem_alloc(sizeof(theme_context_t));
    lv_memset_00(styles, sizeof(theme_context_t));
    styles->ui = ui;

    theme->font_small = ui->font.small;
    theme->font_normal = ui->font.body;
    theme->font_large = ui->font.heading3;

    lv_color_t primary_color = lv_color_hex(STEAM_FOCUS);
    lv_color_t accent_color = lv_color_hex(STEAM_ACCENT);
    lv_color_t focus_color = lv_color_hex(STEAM_FOCUS);
    lv_color_t text_color = lv_color_hex(STEAM_TEXT);
    lv_color_t panel_color = lv_color_hex(STEAM_BG_PANEL);

    theme->color_primary = primary_color;
    theme->color_secondary = accent_color;

    lv_style_init(&styles->scr);

    /* Vertical Steam gradient: deep navy → blue panel tone */
    const static lv_grad_dsc_t grad = {
            .dir = LV_GRAD_DIR_VER,
            .stops = {
                    {.color = {.ch = {.red = 0x17, .green = 0x1a, .blue = 0x21, .alpha = 255}}, .frac = 0},
                    {.color = {.ch = {.red = 0x1b, .green = 0x28, .blue = 0x38, .alpha = 255}}, .frac = 90},
                    {.color = {.ch = {.red = 0x1f, .green = 0x32, .blue = 0x46, .alpha = 255}}, .frac = 170},
                    {.color = {.ch = {.red = 0x2a, .green = 0x47, .blue = 0x5e, .alpha = 255}}, .frac = 255},
            },
            .stops_count = 4,
    };
    lv_style_set_bg_grad(&styles->scr, &grad);
    lv_style_set_bg_opa(&styles->scr, LV_OPA_COVER);

    lv_style_init(&styles->obj);
    lv_style_set_text_color(&styles->obj, text_color);
    lv_style_set_pad_gap(&styles->obj, LV_DPX(10));

    lv_style_init(&styles->focused);
    lv_style_set_outline_width(&styles->focused, LV_DPX(2));
    lv_style_set_outline_opa(&styles->focused, LV_OPA_COVER);
    lv_style_set_outline_color(&styles->focused, accent_color);
    lv_style_set_outline_pad(&styles->focused, LV_DPX(4));
    lv_style_set_radius(&styles->focused, LV_DPX(3));

    lv_style_init(&styles->label);
    lv_style_set_text_font(&styles->label, ui->font.body);
    lv_style_set_text_color(&styles->label, text_color);

    lv_style_init(&styles->btn);
    lv_style_set_pad_all(&styles->btn, LV_DPX(12));
    lv_style_set_radius(&styles->btn, LV_DPX(3));
    lv_style_set_bg_color(&styles->btn, panel_color);
    lv_style_set_bg_opa(&styles->btn, LV_OPA_COVER);
    lv_style_set_border_width(&styles->btn, LV_DPX(1));
    lv_style_set_border_color(&styles->btn, lv_color_hex(0x3d6a8a));
    lv_style_set_border_opa(&styles->btn, LV_OPA_40);
    lv_style_set_text_color(&styles->btn, text_color);

    lv_style_init(&styles->btn_focused);
    lv_style_set_bg_color(&styles->btn_focused, focus_color);
    lv_style_set_bg_opa(&styles->btn_focused, LV_OPA_COVER);
    lv_style_set_border_color(&styles->btn_focused, accent_color);
    lv_style_set_border_opa(&styles->btn_focused, LV_OPA_COVER);
    lv_style_set_text_color(&styles->btn_focused, lv_color_white());
    lv_style_set_shadow_width(&styles->btn_focused, LV_DPX(16));
    lv_style_set_shadow_color(&styles->btn_focused, focus_color);
    lv_style_set_shadow_opa(&styles->btn_focused, LV_OPA_40);

    lv_style_init(&styles->btn_pressed);
    lv_style_set_bg_color(&styles->btn_pressed, lv_color_darken(focus_color, LV_OPA_20));
    lv_style_set_bg_opa(&styles->btn_pressed, LV_OPA_COVER);

    lv_style_init(&styles->dropdown_list);
    lv_style_set_pad_ver(&styles->dropdown_list, LV_DPX(20));
    lv_style_set_pad_hor(&styles->dropdown_list, LV_DPX(15));
    lv_style_set_text_line_space(&styles->dropdown_list, LV_DPX(20));

    lv_style_init(&styles->modal_bg);
    lv_style_set_bg_color(&styles->modal_bg, lv_color_hex(STEAM_BG_DEEP));
    lv_style_set_bg_opa(&styles->modal_bg, LV_OPA_COVER);
    lv_style_set_border_width(&styles->modal_bg, LV_DPX(1));
    lv_style_set_border_color(&styles->modal_bg, panel_color);
    lv_style_set_border_opa(&styles->modal_bg, LV_OPA_COVER);
    lv_style_set_shadow_color(&styles->modal_bg, lv_color_black());
    lv_style_set_shadow_opa(&styles->modal_bg, LV_OPA_50);
    lv_style_set_shadow_width(&styles->modal_bg, LV_DPX(24));
    lv_style_set_shadow_ofs_y(&styles->modal_bg, LV_DPX(12));
    lv_style_set_radius(&styles->modal_bg, LV_DPX(4));
    lv_style_set_min_width(&styles->modal_bg, LV_DPX(480));
    lv_style_set_max_width(&styles->modal_bg, LV_DPX(576));

    lv_style_init(&styles->msgbox);
    lv_style_set_pad_top(&styles->msgbox, LV_DPX(16));
    lv_style_set_pad_bottom(&styles->msgbox, LV_DPX(8));
    lv_style_set_pad_hor(&styles->msgbox, LV_DPX(8));
    lv_style_set_flex_main_place(&styles->msgbox, LV_FLEX_ALIGN_END);

    lv_style_init(&styles->msgbox_backdrop);
    lv_style_set_bg_color(&styles->msgbox_backdrop, lv_color_black());
    lv_style_set_bg_opa(&styles->msgbox_backdrop, LV_OPA_50);

    lv_style_init(&styles->msgbox_title);
    lv_style_set_text_font(&styles->msgbox_title, ui->font.heading2);
    lv_style_set_text_color(&styles->msgbox_title, accent_color);
    lv_style_set_text_letter_space(&styles->msgbox_title, LV_DPX(1));
    lv_style_set_pad_hor(&styles->msgbox_title, LV_DPX(16));

    lv_style_init(&styles->msgbox_text);
    lv_style_set_text_font(&styles->msgbox_text, ui->font.body);
    lv_style_set_text_color(&styles->msgbox_text, text_color);
    lv_style_set_pad_top(&styles->msgbox_text, LV_DPX(8));
    lv_style_set_pad_hor(&styles->msgbox_text, LV_DPX(16));
    lv_style_set_pad_bottom(&styles->msgbox_text, LV_DPX(16));

    lv_style_init(&styles->msgbox_btns);

    lv_style_init(&styles->msgbox_btns_item);
    lv_style_set_radius(&styles->msgbox_btns_item, LV_DPX(2));
    lv_style_set_text_font(&styles->msgbox_btns_item, ui->font.small);
    lv_style_set_text_color(&styles->msgbox_btns_item, accent_color);

    lv_style_init(&styles->msgbox_btns_item_focused);
    lv_style_set_bg_color(&styles->msgbox_btns_item_focused, focus_color);
    lv_style_set_bg_opa(&styles->msgbox_btns_item_focused, LV_OPA_COVER);
    lv_style_set_text_color(&styles->msgbox_btns_item_focused, lv_color_white());

    lv_style_init(&styles->msgbox_btns_item_pressed);
    lv_style_set_bg_color(&styles->msgbox_btns_item_pressed, lv_color_darken(focus_color, LV_OPA_20));
    lv_style_set_bg_opa(&styles->msgbox_btns_item_pressed, LV_OPA_COVER);

    lv_style_init(&styles->win_title);
    lv_style_set_text_font(&styles->win_title, ui->font.heading2);
    lv_style_set_text_color(&styles->win_title, lv_color_white());
    lv_style_set_text_letter_space(&styles->win_title, LV_DPX(2));

    lv_style_init(&styles->win_btn);
    lv_style_set_text_font(&styles->win_btn, ui->iconfont.heading3);
    lv_style_set_radius(&styles->win_btn, LV_DPX(3));
    lv_style_set_min_height(&styles->win_btn, LV_DPX(40));
    lv_style_set_max_height(&styles->win_btn, LV_DPX(40));
    lv_style_set_bg_color(&styles->win_btn, lv_color_hex(STEAM_BG_DEEP));
    lv_style_set_bg_opa(&styles->win_btn, LV_OPA_60);

    lv_style_init(&styles->win_header);
    lv_style_set_min_height(&styles->win_header, LV_DPX(64));
    lv_style_set_pad_hor(&styles->win_header, LV_DPX(32));
    lv_style_set_pad_top(&styles->win_header, LV_DPX(18));
    lv_style_set_pad_bottom(&styles->win_header, LV_DPX(12));
    lv_style_set_bg_color(&styles->win_header, lv_color_hex(STEAM_BG_DEEP));
    lv_style_set_bg_opa(&styles->win_header, LV_OPA_80);
    lv_style_set_border_side(&styles->win_header, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&styles->win_header, LV_DPX(1));
    lv_style_set_border_color(&styles->win_header, lv_color_hex(0x000000));
    lv_style_set_border_opa(&styles->win_header, LV_OPA_40);

    lv_style_init(&styles->win_content);
    lv_style_set_pad_hor(&styles->win_content, LV_DPX(32));
    lv_style_set_pad_top(&styles->win_content, LV_DPX(16));
    lv_style_set_pad_bottom(&styles->win_content, LV_DPX(20));

    lv_style_init(&styles->arc_indic);
    lv_style_set_arc_color(&styles->arc_indic, panel_color);
    lv_style_set_arc_opa(&styles->arc_indic, LV_OPA_COVER);
    lv_style_set_arc_width(&styles->arc_indic, LV_DPX(12));
    lv_style_set_arc_rounded(&styles->arc_indic, true);

    lv_style_init(&styles->arc_indic_primary);
    lv_style_set_arc_color(&styles->arc_indic_primary, accent_color);
    lv_style_set_arc_opa(&styles->arc_indic_primary, LV_OPA_COVER);

    theme->user_data = styles;

    lv_theme_set_apply_cb(theme, apply_cb);
}

void app_theme_deinit(lv_theme_t *theme) {
    theme_context_t *styles = theme->user_data;
    lv_style_reset(&styles->arc_indic_primary);
    lv_style_reset(&styles->arc_indic);

    lv_style_reset(&styles->win_content);
    lv_style_reset(&styles->win_header);
    lv_style_reset(&styles->win_btn);
    lv_style_reset(&styles->win_title);

    lv_style_reset(&styles->msgbox_btns_item_pressed);
    lv_style_reset(&styles->msgbox_btns_item_focused);
    lv_style_reset(&styles->msgbox_btns_item);
    lv_style_reset(&styles->msgbox_btns);
    lv_style_reset(&styles->msgbox_text);
    lv_style_reset(&styles->msgbox_title);
    lv_style_reset(&styles->msgbox_backdrop);
    lv_style_reset(&styles->msgbox);

    lv_style_reset(&styles->modal_bg);

    lv_style_reset(&styles->dropdown_list);

    lv_style_reset(&styles->btn_focused);
    lv_style_reset(&styles->btn_pressed);
    lv_style_reset(&styles->btn);

    lv_style_reset(&styles->focused);
    lv_style_reset(&styles->obj);
    lv_style_reset(&styles->scr);

    free(styles);
}

lv_coord_t app_win_header_size(lv_theme_t *theme) {
    if (theme->apply_cb != apply_cb) {
        return LV_SIZE_CONTENT;
    }
    theme_context_t *styles = theme->user_data;
    lv_style_value_t value;
    if (lv_style_get_prop(&styles->win_header, LV_STYLE_MIN_HEIGHT, &value) != LV_RES_OK) {
        return LV_SIZE_CONTENT;
    }
    return value.num;
}

lv_obj_t *app_lv_win_create(lv_obj_t *parent) {
    lv_theme_t *theme = lv_disp_get_theme(lv_obj_get_disp(parent));
    lv_obj_t *win = lv_win_create(parent, app_win_header_size(theme));
    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_gap(header, LV_DPX(12), 0);
    return win;
}

static void win_close_clicked(lv_event_t *e) {
    app_t *app = lv_event_get_user_data(e);
    app_ui_pop_top_fragment(app->ui);
}

lv_obj_t *app_lv_win_add_close_btn(lv_obj_t *win, app_t *app) {
    lv_obj_t *header = lv_win_get_header(win);
    lv_obj_t *btn = lv_btn_create(header);
    lv_obj_set_height(btn, LV_DPX(44));
    lv_obj_set_style_pad_hor(btn, LV_DPX(18), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(STEAM_BG_PANEL), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(STEAM_FOCUS), LV_STATE_FOCUS_KEY);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btn, LV_DPX(8), 0);

    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text_static(icon, BS_SYMBOL_X_LG);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Закрыть");

    lv_obj_add_event_cb(btn, win_close_clicked, LV_EVENT_CLICKED, app);
    return btn;
}

static void apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
    theme_context_t *styles = theme->user_data;
    lv_obj_t *parent = lv_obj_get_parent(obj);
    if (parent == NULL) {
        lv_obj_add_style(obj, &styles->scr, 0);
        return;
    }
    lv_obj_add_style(obj, &styles->obj, 0);
    if (lv_obj_has_class(obj, &lv_btn_class)) {
        lv_obj_add_style(obj, &styles->btn, 0);
        lv_obj_add_style(obj, &styles->btn_pressed, LV_STATE_PRESSED);
        lv_obj_add_style(obj, &styles->btn_focused, LV_STATE_FOCUS_KEY);
        if (parent->parent != NULL && lv_obj_check_type(parent->parent, &lv_win_class)) {
            if (lv_win_get_header(parent->parent) == obj->parent) {
                lv_obj_add_style(obj, &styles->win_btn, 0);
            }
        }
    } else if (lv_obj_has_class(obj, &lv_label_class)) {
        lv_obj_add_style(obj, &styles->label, 0);
        if (lv_obj_check_type(parent, &lv_msgbox_class)) {
            if (lv_msgbox_get_title(parent) == NULL && lv_msgbox_get_content(parent) == NULL) {
                // Title was not assigned, and the content was not created either. Assume this is the title
                lv_obj_add_style(obj, &styles->msgbox_title, 0);
            }
        } else if (parent->parent != NULL) {
            if (lv_obj_check_type(parent->parent, &lv_msgbox_class)) {
                if (lv_msgbox_get_content(parent->parent) == lv_obj_get_parent(obj)) {
                    lv_obj_add_style(obj, &styles->msgbox_text, 0);
                }
            } else if (lv_obj_check_type(parent->parent, &lv_win_class)) {
                if (lv_win_get_header(parent->parent) == lv_obj_get_parent(obj)) {
                    lv_obj_add_style(obj, &styles->win_title, 0);
                }
            }
        }
    } else if (lv_obj_has_class(obj, &lv_dropdown_class)) {
        lv_obj_add_style(obj, &styles->label, 0);
        lv_obj_add_style(obj, &styles->focused, LV_STATE_FOCUS_KEY);
    } else if (lv_obj_has_class(obj, &lv_dropdownlist_class)) {
        lv_obj_add_style(obj, &styles->modal_bg, 0);
        lv_obj_add_style(obj, &styles->modal_bg, 0);
        lv_obj_add_style(obj, &styles->label, LV_PART_SELECTED);
        lv_obj_add_style(obj, &styles->dropdown_list, 0);
        lv_obj_add_style(obj, &styles->dropdown_list, LV_PART_SELECTED);
    } else if (lv_obj_has_class(obj, &lv_btnmatrix_class)) {
        if (lv_obj_check_type(parent, &lv_msgbox_class)) {
            lv_obj_add_style(obj, &styles->msgbox_btns, 0);
            lv_obj_add_style(obj, &styles->msgbox_btns_item, LV_PART_ITEMS);
            lv_obj_add_style(obj, &styles->msgbox_btns_item_focused, LV_PART_ITEMS | LV_STATE_FOCUSED);
            lv_obj_add_style(obj, &styles->msgbox_btns_item_pressed, LV_PART_ITEMS | LV_STATE_PRESSED);
        }
    } else if (lv_obj_check_type(obj, &lv_msgbox_class)) {
        lv_obj_add_style(obj, &styles->modal_bg, 0);
        lv_obj_add_style(obj, &styles->msgbox, 0);
        msgbox_inject_nav(styles->ui, obj);
    } else if (lv_obj_check_type(obj, &lv_msgbox_backdrop_class)) {
        lv_obj_add_style(obj, &styles->msgbox_backdrop, 0);
    } else if (lv_obj_check_type(obj, &lv_spinner_class)) {
        lv_obj_add_style(obj, &styles->arc_indic, 0);
        lv_obj_add_style(obj, &styles->arc_indic, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &styles->arc_indic_primary, LV_PART_INDICATOR);
    } else if (lv_obj_check_type(obj, &lv_arc_class)) {
        lv_obj_add_style(obj, &styles->arc_indic, 0);
        lv_obj_add_style(obj, &styles->arc_indic, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &styles->arc_indic_primary, LV_PART_INDICATOR);
    } else if (lv_obj_check_type(parent, &lv_win_class)) {
        if (obj == lv_win_get_header(parent)) {
            lv_obj_add_style(obj, &styles->win_header, 0);
        } else if (obj == lv_win_get_content(parent)) {
            lv_obj_add_style(obj, &styles->win_content, 0);
        }
    }
}
