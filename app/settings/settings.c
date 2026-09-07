#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "app_settings.h"

#include "ss4s_modules.h"
#include "array_list.h"
#include "os_info.h"
#include "logging.h"

static void settings_path(char *out, size_t out_len) {
    char *pref = SDL_GetPrefPath("IHSplay", "ihsplay");
    if (pref == NULL) {
        snprintf(out, out_len, "settings.ini");
        return;
    }
    snprintf(out, out_len, "%ssettings.ini", pref);
    SDL_free(pref);
}

static void settings_load(app_settings_t *settings) {
    char path[512];
    settings_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
        char key[64] = {0};
        char value[64] = {0};
        if (sscanf(line, "%63[^=]=%63s", key, value) != 2) {
            continue;
        }
        if (strcmp(key, "audio_module") == 0) {
            snprintf(settings->audio_module_pref, sizeof(settings->audio_module_pref), "%s", value);
        } else if (strcmp(key, "video_module") == 0) {
            snprintf(settings->video_module_pref, sizeof(settings->video_module_pref), "%s", value);
        } else if (strcmp(key, "language") == 0) {
            if (strcmp(value, "ru") == 0) {
                snprintf(settings->language, sizeof(settings->language), "ru");
            } else {
                snprintf(settings->language, sizeof(settings->language), "en");
            }
        } else if (strcmp(key, "magic_remote_onboarding") == 0) {
            settings->magic_remote_onboarding_done = (strcmp(value, "1") == 0 ||
                                                     strcmp(value, "true") == 0);
        }
    }
    fclose(f);
}

void app_settings_save(const app_settings_t *settings) {
    char path[512];
    settings_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        commons_log_warn("Settings", "Can't write %s", path);
        return;
    }
    fprintf(f, "audio_module=%s\n", settings->audio_module_pref[0] ? settings->audio_module_pref : "auto");
    fprintf(f, "video_module=%s\n", settings->video_module_pref[0] ? settings->video_module_pref : "auto");
    fprintf(f, "language=%s\n", settings->language[0] ? settings->language : "en");
    fprintf(f, "magic_remote_onboarding=%s\n", settings->magic_remote_onboarding_done ? "1" : "0");
    fclose(f);
}

static void settings_apply_selection(app_settings_t *settings) {
    SS4S_ModulePreferences preferences = {
            .audio_module = settings->audio_module_pref[0] ? settings->audio_module_pref : MODULE_PREFERENCE_AUTO,
            .video_module = settings->video_module_pref[0] ? settings->video_module_pref : MODULE_PREFERENCE_AUTO,
    };
    SS4S_ModuleSelection selection = {.audio_module = NULL, .video_module = NULL};
    SS4S_ModulesSelect(&settings->modules, &preferences, &selection, true);
    settings->video_driver = SS4S_ModuleInfoGetId(selection.video_module);
    settings->audio_driver = SS4S_ModuleInfoGetId(selection.audio_module);
    if (settings->video_driver == NULL) {
        settings->video_driver = "auto";
    }
    if (settings->audio_driver == NULL) {
        settings->audio_driver = "auto";
    }
}

void app_settings_init(app_settings_t *settings, const os_info_t *os_info) {
    memset(settings, 0, sizeof(app_settings_t));
    snprintf(settings->audio_module_pref, sizeof(settings->audio_module_pref), "%s", MODULE_PREFERENCE_AUTO);
    snprintf(settings->video_module_pref, sizeof(settings->video_module_pref), "%s", MODULE_PREFERENCE_AUTO);
    snprintf(settings->language, sizeof(settings->language), "en");
    settings_load(settings);

    int err;
    if ((err = SS4S_ModulesList(&settings->modules, os_info)) != 0) {
        commons_log_error("SS4S", "Can't load modules list: %s", strerror(err));
    }
    settings->enable_input = true;
    settings->relmouse = true;
    settings_apply_selection(settings);
    commons_log_info("Settings", "Audio pref=%s -> %s, video pref=%s -> %s",
                     settings->audio_module_pref, settings->audio_driver ? settings->audio_driver : "?",
                     settings->video_module_pref, settings->video_driver ? settings->video_driver : "?");
}

void app_settings_deinit(app_settings_t *settings) {
    SS4S_ModulesListClear(&settings->modules);
}

bool app_settings_set_audio_pref(app_settings_t *settings, const char *module_id_or_auto) {
    if (settings == NULL || module_id_or_auto == NULL || module_id_or_auto[0] == '\0') {
        return false;
    }
    snprintf(settings->audio_module_pref, sizeof(settings->audio_module_pref), "%s", module_id_or_auto);
    settings_apply_selection(settings);
    app_settings_save(settings);
    return true;
}

bool app_settings_set_video_pref(app_settings_t *settings, const char *module_id_or_auto) {
    if (settings == NULL || module_id_or_auto == NULL || module_id_or_auto[0] == '\0') {
        return false;
    }
    snprintf(settings->video_module_pref, sizeof(settings->video_module_pref), "%s", module_id_or_auto);
    settings_apply_selection(settings);
    app_settings_save(settings);
    return true;
}

bool app_settings_set_language(app_settings_t *settings, const char *lang_en_or_ru) {
    if (settings == NULL || lang_en_or_ru == NULL) {
        return false;
    }
    if (strcmp(lang_en_or_ru, "ru") == 0) {
        snprintf(settings->language, sizeof(settings->language), "ru");
    } else {
        snprintf(settings->language, sizeof(settings->language), "en");
    }
    app_settings_save(settings);
    return true;
}

void app_settings_set_magic_remote_onboarding_done(app_settings_t *settings, bool done) {
    if (settings == NULL) {
        return;
    }
    settings->magic_remote_onboarding_done = done;
    app_settings_save(settings);
}
