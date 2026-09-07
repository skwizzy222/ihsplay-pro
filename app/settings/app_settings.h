#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "array_list.h"

typedef struct os_info_t os_info_t;

typedef struct app_settings_t {
    bool enable_input;
    bool relmouse;
    /** Active SS4S module ids (pointers into modules list, or into pref buffers). */
    const char *audio_driver;
    const char *video_driver;
    /** User preference: "auto" or module id. Persisted. */
    char audio_module_pref[64];
    char video_module_pref[64];
    /** UI language: "en" (default) or "ru". */
    char language[8];
    array_list_t modules;
    uint64_t selected_client_id;
} app_settings_t;

void app_settings_init(app_settings_t *settings, const os_info_t *os_info);

void app_settings_deinit(app_settings_t *settings);

/** Persist prefs and recompute active drivers (SS4S needs app restart to switch). */
bool app_settings_set_audio_pref(app_settings_t *settings, const char *module_id_or_auto);

bool app_settings_set_video_pref(app_settings_t *settings, const char *module_id_or_auto);

bool app_settings_set_language(app_settings_t *settings, const char *lang_en_or_ru);

void app_settings_save(const app_settings_t *settings);
