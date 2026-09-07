#pragma once

#include <string.h>
#include <stdbool.h>

#include "app.h"

static inline bool app_lang_is_ru(const app_t *app) {
    return app != NULL && app->settings != NULL &&
           strcmp(app->settings->language, "ru") == 0;
}

#define APP_TR(app, en, ru) (app_lang_is_ru(app) ? (ru) : (en))
