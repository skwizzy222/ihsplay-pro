#pragma once

#include <lvgl.h>

typedef struct app_ui_t app_ui_t;
typedef struct app_t app_t;

void app_theme_init(lv_theme_t *theme, app_ui_t *ui);

void app_theme_deinit(lv_theme_t *theme);

lv_coord_t app_win_header_size(lv_theme_t *theme);

lv_obj_t *app_lv_win_create(lv_obj_t *parent);

/** Labeled Close control for secondary screens (not used during active stream). */
lv_obj_t *app_lv_win_add_close_btn(lv_obj_t *win, app_t *app);
