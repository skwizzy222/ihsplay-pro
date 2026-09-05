#include "app.h"

void app_preinit(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT, "true");
    /* Always receive Magic Remote Back so UI and stream can handle "назад". */
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
}

void app_ui_set_handle_nav_back(app_ui_t *ui, bool handle) {
    (void) ui;
    (void) handle;
    /* Keep Back enabled permanently — see app_preinit. */
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
}