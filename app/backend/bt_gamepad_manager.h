#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "array_list.h"

typedef struct bt_gamepad_device_t {
    char address[24];
    char name[96];
    bool paired;
    bool connected;
    bool connected_hid;
    bool likely_gamepad;
} bt_gamepad_device_t;

bool bt_gamepad_available(void);

bool bt_gamepad_start_scan(void);

bool bt_gamepad_stop_scan(void);

/**
 * Fills @p out (array_list of bt_gamepad_device_t) with HID/gamepad-like devices only.
 */
bool bt_gamepad_refresh_devices(array_list_t *out);

/**
 * Pair (if needed) then HID-connect. Verifies HID profile afterwards.
 */
bool bt_gamepad_connect(const char *address, char *err_buf, size_t err_buf_len);

bool bt_gamepad_disconnect(const char *address, char *err_buf, size_t err_buf_len);

bool bt_gamepad_unpair(const char *address, char *err_buf, size_t err_buf_len);
