#include "backend/bt_gamepad_manager.h"

#include <stdio.h>
#include <string.h>

bool bt_gamepad_available(void) {
    return false;
}

bool bt_gamepad_start_scan(void) {
    return false;
}

bool bt_gamepad_stop_scan(void) {
    return false;
}

bool bt_gamepad_refresh_devices(array_list_t *out) {
    if (out == NULL) {
        return false;
    }
    while (array_list_size(out) > 0) {
        array_list_remove(out, 0);
    }
    return true;
}

bool bt_gamepad_connect(const char *address, char *err_buf, size_t err_buf_len) {
    (void) address;
    if (err_buf && err_buf_len) {
        snprintf(err_buf, err_buf_len, "Bluetooth-геймпады доступны только на webOS TV");
    }
    return false;
}

bool bt_gamepad_disconnect(const char *address, char *err_buf, size_t err_buf_len) {
    (void) address;
    if (err_buf && err_buf_len) {
        snprintf(err_buf, err_buf_len, "Bluetooth-геймпады доступны только на webOS TV");
    }
    return false;
}
