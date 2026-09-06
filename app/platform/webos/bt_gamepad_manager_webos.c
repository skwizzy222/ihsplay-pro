#define _GNU_SOURCE

#include "backend/bt_gamepad_manager.h"

#include "logging.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>

#include <pbnjson.h>
#include "lunasynccall.h"

#define BT_ADAPTER "luna://com.webos.service.bluetooth2/adapter"
#define BT_DEVICE "luna://com.webos.service.bluetooth2/device"
#define BT_HID "luna://com.webos.service.bluetooth2/hid"

static bool luna_json_ok(const char *payload) {
    if (payload == NULL) {
        return false;
    }
    return strstr(payload, "\"returnValue\":true") != NULL ||
           strstr(payload, "\"returnValue\": true") != NULL;
}

static bool luna_call(const char *uri, const char *payload, char **out) {
    char *raw = NULL;
    if (!HLunaServiceCallSync(uri, payload, true, &raw) || raw == NULL) {
        commons_log_warn("BTGamepad", "Call failed: %s", uri);
        if (out) {
            *out = NULL;
        }
        free(raw);
        return false;
    }
    bool ok = luna_json_ok(raw);
    if (!ok) {
        commons_log_warn("BTGamepad", "%s => %s", uri, raw);
    }
    if (out) {
        *out = raw;
    } else {
        free(raw);
    }
    return ok;
}

static bool json_bool(jvalue_ref obj, const char *key, bool def) {
    jvalue_ref v = jobject_get(obj, j_cstr_to_buffer(key));
    if (!jis_boolean(v)) {
        return def;
    }
    bool out = def;
    jboolean_get(v, &out);
    return out;
}

static bool json_string_copy(jvalue_ref obj, const char *key, char *dst, size_t dst_len) {
    jvalue_ref v = jobject_get(obj, j_cstr_to_buffer(key));
    if (!jis_string(v)) {
        return false;
    }
    raw_buffer b = jstring_get(v);
    if (b.m_len == 0 || b.m_str == NULL) {
        return false;
    }
    snprintf(dst, dst_len, "%.*s", (int) b.m_len, b.m_str);
    return true;
}

static bool profiles_has(jvalue_ref device, const char *needle) {
    jvalue_ref profiles = jobject_get(device, j_cstr_to_buffer("connectedProfiles"));
    if (!jis_array(profiles)) {
        return false;
    }
    for (ssize_t i = 0; i < jarray_size(profiles); i++) {
        jvalue_ref p = jarray_get(profiles, i);
        if (!jis_string(p)) {
            continue;
        }
        raw_buffer pb = jstring_get(p);
        if (pb.m_str && strcasecmp(pb.m_str, needle) == 0) {
            return true;
        }
    }
    return false;
}

static bool name_looks_audio(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    static const char *bad[] = {
            "headphone", "headset", "earbud", "buds", "airpods", "speaker",
            "soundbar", "wh-", "wf-", "travel", "galaxy buds", "freebuds", NULL
    };
    char lower[96];
    size_t n = strlen(name);
    if (n >= sizeof(lower)) {
        n = sizeof(lower) - 1;
    }
    for (size_t i = 0; i < n; i++) {
        lower[i] = (char) tolower((unsigned char) name[i]);
    }
    lower[n] = '\0';
    for (int i = 0; bad[i]; i++) {
        if (strstr(lower, bad[i])) {
            return true;
        }
    }
    return false;
}

static bool name_looks_gamepad(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    static const char *good[] = {
            "gamepad", "controller", "joystick", "dualshock", "dualsense",
            "xbox", "wireless controller", "8bitdo", "gamespad", "joy-con",
            "pro controller", "stadia", "backbone", NULL
    };
    char lower[96];
    size_t n = strlen(name);
    if (n >= sizeof(lower)) {
        n = sizeof(lower) - 1;
    }
    for (size_t i = 0; i < n; i++) {
        lower[i] = (char) tolower((unsigned char) name[i]);
    }
    lower[n] = '\0';
    for (int i = 0; good[i]; i++) {
        if (strstr(lower, good[i])) {
            return true;
        }
    }
    return false;
}

/* Bluetooth CoD: major class = (cod >> 8) & 0x1F. 5 = Peripheral, 4 = Audio/Video */
static int cod_major(uint32_t cod) {
    return (int) ((cod >> 8) & 0x1F);
}

static bool device_is_gamepad_candidate(jvalue_ref device, char *name_out, size_t name_len,
                                        bool *out_hid, bool *out_likely) {
    char name[96] = {0};
    char type[32] = {0};
    (void) json_string_copy(device, "name", name, sizeof(name));
    /* Some firmwares put display name elsewhere */
    if (name[0] == '\0') {
        (void) json_string_copy(device, "remoteName", name, sizeof(name));
    }
    if (name[0] == '\0') {
        (void) json_string_copy(device, "aliasName", name, sizeof(name));
    }
    (void) json_string_copy(device, "typeOfDevice", type, sizeof(type));

    bool paired = json_bool(device, "paired", false);
    bool has_hid = profiles_has(device, "hid");
    bool has_gatt = profiles_has(device, "gatt");
    bool has_a2dp = profiles_has(device, "a2dp");

    jvalue_ref cod_ref = jobject_get(device, j_cstr_to_buffer("classOfDevice"));
    uint32_t cod = 0;
    if (jis_number(cod_ref)) {
        jnumber_get_i32(cod_ref, (int32_t *) &cod);
    }
    int major = cod_major(cod);
    bool peripheral = (major == 5);
    bool audio_major = (major == 4);

    if (name_out && name_len) {
        if (name[0]) {
            snprintf(name_out, name_len, "%s", name);
        } else {
            name_out[0] = '\0';
        }
    }
    if (out_hid) {
        *out_hid = has_hid;
    }

    /* Never show anonymous BLE beacons / Apple Continuity / GATT-only noise */
    if (strcasecmp(type, "ble") == 0 && name[0] == '\0') {
        return false;
    }
    if (has_gatt && !has_hid && !peripheral && !name_looks_gamepad(name)) {
        return false;
    }
    if (has_a2dp && !has_hid && !peripheral) {
        return false;
    }
    if (audio_major || name_looks_audio(name)) {
        return false;
    }

    bool likely = has_hid || peripheral || name_looks_gamepad(name);
    if (out_likely) {
        *out_likely = likely;
    }

    if (has_hid || peripheral || name_looks_gamepad(name)) {
        return true;
    }
    /* Named classic BT that is already paired — show so user can manage, but mark not likely */
    if (paired && name[0] != '\0' && (strcasecmp(type, "bredr") == 0 || strcasecmp(type, "dual") == 0)) {
        if (out_likely) {
            *out_likely = false;
        }
        return true;
    }
    return false;
}

bool bt_gamepad_available(void) {
    return true;
}

bool bt_gamepad_start_scan(void) {
    (void) luna_call(BT_ADAPTER "/setState", "{\"powered\":true,\"discoverable\":true}", NULL);
    return luna_call(BT_ADAPTER "/startDiscovery", "{}", NULL);
}

bool bt_gamepad_stop_scan(void) {
    return luna_call(BT_ADAPTER "/cancelDiscovery", "{}", NULL);
}

static void parse_device(jvalue_ref device, bt_gamepad_device_t *out, const char *name,
                         bool has_hid, bool likely) {
    memset(out, 0, sizeof(*out));
    (void) json_string_copy(device, "address", out->address, sizeof(out->address));
    if (name && name[0]) {
        snprintf(out->name, sizeof(out->name), "%s", name);
    } else {
        snprintf(out->name, sizeof(out->name), "Без имени (%s)",
                 out->address[0] ? out->address : "?");
    }
    out->paired = json_bool(device, "paired", false);
    out->connected = json_bool(device, "connected", false);
    out->connected_hid = has_hid;
    out->likely_gamepad = likely;
    if (has_hid) {
        out->connected = true;
    }
}

bool bt_gamepad_refresh_devices(array_list_t *out) {
    if (out == NULL) {
        return false;
    }
    while (array_list_size(out) > 0) {
        array_list_remove(out, 0);
    }

    char *raw = NULL;
    if (!luna_call(BT_DEVICE "/getStatus", "{}", &raw) || raw == NULL) {
        free(raw);
        return false;
    }

    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
    jdomparser_feed(parser, raw, (int) strlen(raw));
    jdomparser_end(parser);
    jvalue_ref body = jdomparser_get_result(parser);
    jvalue_ref devices = jobject_get(body, j_cstr_to_buffer("devices"));
    if (!jis_array(devices)) {
        devices = jobject_get(body, j_cstr_to_buffer("deviceList"));
    }
    if (jis_array(devices)) {
        for (ssize_t i = 0; i < jarray_size(devices); i++) {
            jvalue_ref d = jarray_get(devices, i);
            if (!jis_object(d)) {
                continue;
            }
            char name[96] = {0};
            bool has_hid = false;
            bool likely = false;
            if (!device_is_gamepad_candidate(d, name, sizeof(name), &has_hid, &likely)) {
                continue;
            }
            bt_gamepad_device_t parsed;
            parse_device(d, &parsed, name, has_hid, likely);
            if (parsed.address[0] == '\0') {
                continue;
            }
            bt_gamepad_device_t *slot = array_list_add(out, -1);
            *slot = parsed;
        }
    }
    jdomparser_release(&parser);
    free(raw);
    return true;
}

static bool address_has_hid(const char *address) {
    array_list_t list;
    array_list_init(&list, sizeof(bt_gamepad_device_t), 8);
    bool ok = false;
    if (bt_gamepad_refresh_devices(&list)) {
        for (int i = 0, n = (int) array_list_size(&list); i < n; i++) {
            bt_gamepad_device_t *d = array_list_get(&list, i);
            if (strcasecmp(d->address, address) == 0 && d->connected_hid) {
                ok = true;
                break;
            }
        }
    }
    array_list_deinit(&list);
    return ok;
}

bool bt_gamepad_connect(const char *address, char *err_buf, size_t err_buf_len) {
    if (address == NULL || address[0] == '\0') {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "Нет адреса устройства");
        }
        return false;
    }

    array_list_t list;
    array_list_init(&list, sizeof(bt_gamepad_device_t), 8);
    bool paired = false;
    bool likely = false;
    if (bt_gamepad_refresh_devices(&list)) {
        for (int i = 0, n = (int) array_list_size(&list); i < n; i++) {
            bt_gamepad_device_t *d = array_list_get(&list, i);
            if (strcasecmp(d->address, address) == 0) {
                paired = d->paired;
                likely = d->likely_gamepad;
                if (d->connected_hid) {
                    array_list_deinit(&list);
                    return true;
                }
                break;
            }
        }
    }
    array_list_deinit(&list);

    if (!likely) {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len,
                     "Это не похоже на геймпад (наушники/другое BT). Подключение отменено.");
        }
        return false;
    }

    (void) bt_gamepad_stop_scan();

    if (!paired) {
        char payload[160];
        snprintf(payload, sizeof(payload), "{\"address\":\"%s\",\"subscribe\":true}", address);
        char *raw = NULL;
        (void) luna_call(BT_ADAPTER "/pair", payload, &raw);
        free(raw);
        usleep(2500000);
    }

    char payload[128];
    snprintf(payload, sizeof(payload), "{\"address\":\"%s\"}", address);
    char *raw = NULL;
    bool ok = luna_call(BT_HID "/connect", payload, &raw);
    free(raw);
    if (!ok) {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "HID connect не удался");
        }
        return false;
    }
    usleep(800000);
    if (!address_has_hid(address)) {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len,
                     "Устройство ответило, но HID-геймпад не появился. Это не контроллер?");
        }
        return false;
    }
    return true;
}

bool bt_gamepad_disconnect(const char *address, char *err_buf, size_t err_buf_len) {
    if (address == NULL || address[0] == '\0') {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "Нет адреса устройства");
        }
        return false;
    }
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"address\":\"%s\"}", address);
    if (!luna_call(BT_HID "/disconnect", payload, NULL)) {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "Не удалось отключить");
        }
        return false;
    }
    return true;
}

bool bt_gamepad_unpair(const char *address, char *err_buf, size_t err_buf_len) {
    if (address == NULL || address[0] == '\0') {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "Нет адреса устройства");
        }
        return false;
    }
    (void) bt_gamepad_disconnect(address, NULL, 0);
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"address\":\"%s\"}", address);
    if (!luna_call(BT_ADAPTER "/unpair", payload, NULL)) {
        /* Some firmwares use device/unpair */
        if (!luna_call(BT_DEVICE "/unpair", payload, NULL)) {
            if (err_buf && err_buf_len) {
                snprintf(err_buf, err_buf_len, "Не удалось забыть устройство");
            }
            return false;
        }
    }
    return true;
}
