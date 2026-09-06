#include "app.h"
#include "basic.h"

#include "ss4s_modules.h"
#include "array_list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct basic_fragment {
    lv_fragment_t base;
    app_t *app;
    lv_obj_t *audio_dd;
    lv_obj_t *video_dd;
    char **audio_ids;
    int audio_count;
    char **video_ids;
    int video_count;
} basic_fragment;

static void free_id_list(char **ids, int count) {
    if (ids == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(ids[i]);
    }
    free(ids);
}

static void restart_hint_close(lv_event_t *e) {
    lv_msgbox_close_async(lv_event_get_current_target(e));
}

static void show_restart_hint(void) {
    static const char *btns[] = {"OK", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Настройки",
                                      "Модуль сохранён. Полностью закройте приложение и откройте снова, "
                                      "чтобы аудио/видео переключились.",
                                      btns, false);
    lv_obj_add_event_cb(mbox, restart_hint_close, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
}

static void audio_changed(lv_event_t *e) {
    basic_fragment *fragment = lv_event_get_user_data(e);
    uint16_t idx = lv_dropdown_get_selected(fragment->audio_dd);
    if (idx >= (uint16_t) fragment->audio_count) {
        return;
    }
    app_settings_set_audio_pref(fragment->app->settings, fragment->audio_ids[idx]);
    show_restart_hint();
}

static void video_changed(lv_event_t *e) {
    basic_fragment *fragment = lv_event_get_user_data(e);
    uint16_t idx = lv_dropdown_get_selected(fragment->video_dd);
    if (idx >= (uint16_t) fragment->video_count) {
        return;
    }
    app_settings_set_video_pref(fragment->app->settings, fragment->video_ids[idx]);
    show_restart_hint();
}

static void constructor(lv_fragment_t *self, void *arg) {
    basic_fragment *fragment = (basic_fragment *) self;
    fragment->app = arg;
}

static void destructor(lv_fragment_t *self) {
    basic_fragment *fragment = (basic_fragment *) self;
    free_id_list(fragment->audio_ids, fragment->audio_count);
    free_id_list(fragment->video_ids, fragment->video_count);
}

static lv_obj_t *make_select_row(lv_obj_t *parent, const char *title, lv_obj_t **dd_out) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(row, LV_DPX(6), 0);
    lv_obj_set_style_pad_ver(row, LV_DPX(10), 0);
    lv_obj_set_style_pad_hor(row, LV_DPX(12), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1b2838), 0);
    lv_obj_set_style_radius(row, LV_DPX(8), 0);

    lv_obj_t *t = lv_label_create(row);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_opa(t, LV_OPA_70, 0);

    lv_obj_t *dd = lv_dropdown_create(row);
    lv_obj_set_width(dd, LV_PCT(100));
    *dd_out = dd;
    return row;
}

static void fill_module_dropdown(basic_fragment *fragment, bool audio) {
    app_settings_t *settings = fragment->app->settings;
    array_list_t *modules = &settings->modules;

    int cap = (int) array_list_size(modules) + 1;
    char **ids = calloc((size_t) cap, sizeof(char *));
    char options[1024];
    size_t opt_len = 0;
    options[0] = '\0';

    int count = 0;
    ids[count++] = strdup("auto");
    opt_len += (size_t) snprintf(options + opt_len, sizeof(options) - opt_len, "Авто (рекомендуется)");

    uint16_t selected = 0;
    const char *pref = audio ? settings->audio_module_pref : settings->video_module_pref;

    for (int i = 0, n = (int) array_list_size(modules); i < n; i++) {
        const SS4S_ModuleInfo *info = array_list_get(modules, i);
        if (audio && !info->has_audio) {
            continue;
        }
        if (!audio && !info->has_video) {
            continue;
        }
        const char *id = SS4S_ModuleInfoGetId(info);
        const char *name = SS4S_ModuleInfoGetName(info);
        if (id == NULL) {
            continue;
        }
        ids[count] = strdup(id);
        if (opt_len + 2 < sizeof(options)) {
            options[opt_len++] = '\n';
            options[opt_len] = '\0';
        }
        const char *label = (name && name[0]) ? name : id;
        opt_len += (size_t) snprintf(options + opt_len, sizeof(options) - opt_len, "%s (%s)", label, id);
        if (pref[0] && strcmp(pref, id) == 0) {
            selected = (uint16_t) count;
        }
        count++;
    }

    if (audio) {
        free_id_list(fragment->audio_ids, fragment->audio_count);
        fragment->audio_ids = ids;
        fragment->audio_count = count;
        lv_dropdown_set_options(fragment->audio_dd, options);
        lv_dropdown_set_selected(fragment->audio_dd, selected);
    } else {
        free_id_list(fragment->video_ids, fragment->video_count);
        fragment->video_ids = ids;
        fragment->video_count = count;
        lv_dropdown_set_options(fragment->video_dd, options);
        lv_dropdown_set_selected(fragment->video_dd, selected);
    }
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    basic_fragment *fragment = (basic_fragment *) self;

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
                      "Если нет звука или чёрный экран — попробуйте другой модуль.\n"
                      "После смены полностью перезапустите приложение.");
    lv_label_set_long_mode(intro, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(intro, LV_PCT(100));

    make_select_row(list, "Аудиовыход", &fragment->audio_dd);
    make_select_row(list, "Видеодекодер", &fragment->video_dd);
    fill_module_dropdown(fragment, true);
    fill_module_dropdown(fragment, false);

    lv_obj_add_event_cb(fragment->audio_dd, audio_changed, LV_EVENT_VALUE_CHANGED, fragment);
    lv_obj_add_event_cb(fragment->video_dd, video_changed, LV_EVENT_VALUE_CHANGED, fragment);

    lv_obj_t *active = lv_label_create(list);
    char buf[160];
    snprintf(buf, sizeof(buf), "Сейчас активно: аудио «%s», видео «%s»",
             fragment->app->settings->audio_driver ? fragment->app->settings->audio_driver : "?",
             fragment->app->settings->video_driver ? fragment->app->settings->video_driver : "?");
    lv_label_set_text(active, buf);
    lv_label_set_long_mode(active, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(active, LV_PCT(100));
    lv_obj_set_style_text_opa(active, LV_OPA_70, 0);

    lv_obj_t *nav = lv_label_create(list);
    lv_label_set_text(nav, "Советы по лагу — в разделе «Поддержка → Советы».");
    lv_label_set_long_mode(nav, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(nav, LV_PCT(100));

    return list;
}

const lv_fragment_class_t settings_basic_fragment_class = {
        .constructor_cb = constructor,
        .destructor_cb = destructor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(basic_fragment),
};
