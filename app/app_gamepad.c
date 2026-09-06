#include "ui/app_ui.h"

#include "lvgl/keypad.h"
#include "app.h"
#include "backend/input_manager.h"
#include "backend/stream_manager.h"

#include <SDL.h>

void app_sdl_input_event(app_t *app, const SDL_Event *event) {
    switch (event->type) {
        case SDL_CONTROLLERDEVICEADDED: {
            input_manager_sdl_gamepad_added(app->input_manager, event->cdevice.which);
            app_post_event(app, APP_UI_GAMEPAD_DEVICE_CHANGED, NULL, NULL);
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED: {
            input_manager_sdl_gamepad_removed(app->input_manager, event->cdevice.which);
            app_post_event(app, APP_UI_GAMEPAD_DEVICE_CHANGED, NULL, NULL);
            break;
        }
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            app_indev_keypad_sdl_key_event(app->ui->indev.keypad, &event->key);
            break;
        }
        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERBUTTONDOWN: {
            app_indev_keypad_sdl_cbutton_event(app->ui->indev.keypad, &event->cbutton);
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            /* Left stick navigates menus when not in a stream (D-pad already maps to keys). */
            if (stream_manager_is_active(app->stream_manager)) {
                break;
            }
            const SDL_ControllerAxisEvent *ax = &event->caxis;
            if (ax->axis != SDL_CONTROLLER_AXIS_LEFTX && ax->axis != SDL_CONTROLLER_AXIS_LEFTY) {
                break;
            }
            const Sint16 dead = 16000;
            static lv_key_t held = 0;
            lv_key_t want = 0;
            if (ax->axis == SDL_CONTROLLER_AXIS_LEFTY) {
                if (ax->value < -dead) {
                    want = LV_KEY_UP;
                } else if (ax->value > dead) {
                    want = LV_KEY_DOWN;
                }
            } else {
                if (ax->value < -dead) {
                    want = LV_KEY_LEFT;
                } else if (ax->value > dead) {
                    want = LV_KEY_RIGHT;
                }
            }
            if (want != held) {
                if (held) {
                    app_indev_keypad_inject_key(app->ui->indev.keypad, held, false);
                }
                if (want) {
                    app_indev_keypad_inject_key(app->ui->indev.keypad, want, true);
                }
                held = want;
            }
            break;
        }
    }
}
