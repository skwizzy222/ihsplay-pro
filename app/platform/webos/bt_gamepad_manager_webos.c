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

static char g_adapter[24] = {0};

static bool luna_json_ok(const char *payload) {
    if (payload == NULL) {
        return false;
    }
    return strstr(payload, "\"returnValue\":true") != NULL ||
           strstr(payload, "\"returnValue\": true") != NULL;
}

static void remember_adapter(const char *payload) {
    if (payload == NULL || g_adapter[0]) {
        return;
    }
    const char *p = strstr(payload, "\"adapterAddress\"");
    if (!p) {
        return;
    }
    p = strchr(p, ':');
    if (!p) {
        return;
    }
    p = strchr(p, '"');
    if (!p) {
        return;
    }
    p++;
    const char *end = strchr(p, '"');
    if (!end || (size_t) (end - p) >= sizeof(g_adapter)) {
        return;
    }
    memcpy(g_adapter, p, (size_t) (end - p));
    g_adapter[end - p] = '\0';
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
    remember_adapter(raw);
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

/**
 * Pairing/HID need a live LS2 subscription until endPairing / connected.
 * Do NOT use luna-send: on TV it is root-only (-rwx------), so popen fails instantly.
 */
static bool luna_subscribe_wait(const char *uri, const char *payload, const char *success_needle,
                                int timeout_sec, char *err_buf, size_t err_buf_len) {
    commons_log_info("BTGamepad", "subscribe: %s", uri);
    bool ok = HLunaServiceCallSyncSubscribe(uri, payload, true, success_needle, timeout_sec, err_buf,
                                            err_buf_len);
    commons_log_info("BTGamepad", "subscribe done ok=%d needle=%s", (int) ok,
                     success_needle ? success_needle : "");
    return ok;
}

static void json_with_adapter(char *dst, size_t dst_len, const char *address, bool subscribe) {
    if (g_adapter[0]) {
        if (subscribe) {
            snprintf(dst, dst_len,
                     "{\"adapterAddress\":\"%s\",\"address\":\"%s\",\"subscribe\":true}",
                     g_adapter, address);
        } else {
            snprintf(dst, dst_len,
                     "{\"adapterAddress\":\"%s\",\"address\":\"%s\"}", g_adapter, address);
        }
    } else if (subscribe) {
        snprintf(dst, dst_len, "{\"address\":\"%s\",\"subscribe\":true}", address);
    } else {
        snprintf(dst, dst_len, "{\"address\":\"%s\"}", address);
    }
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
            "soundbar", "wh-", "wf-", "wi-", "travel", "galaxy buds", "freebuds",
            "koss", "plug wireless", "porta pro", NULL
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
            "xbox", "wireless controller", "8bitdo", "gamepad", "joy-con",
            "pro controller", "stadia", "backbone", "games sir", "gamesir",
            "gamespad", "ipega", "flydigi", "gulikit", NULL
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

static int cod_major(uint32_t cod) {
    return (int) ((cod >> 8) & 0x1F);
}

static bool device_is_gamepad_candidate(jvalue_ref device, char *name_out, size_t name_len,
                                        bool *out_hid, bool *out_likely) {
    char name[96] = {0};
    char type[32] = {0};
    (void) json_string_copy(device, "name", name, sizeof(name));
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
    int32_t cod_i = 0;
    uint32_t cod = 0;
    if (jis_number(cod_ref)) {
        jnumber_get_i32(cod_ref, &cod_i);
        cod = (uint32_t) cod_i;
    }
    int major = cod_major(cod);
    bool peripheral = (major == 5);
    bool audio_major = (major == 4);
    bool phone_major = (major == 2);
    bool computer_major = (major == 1);

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

    if (strcasecmp(type, "ble") == 0 && name[0] == '\0') {
        return false;
    }
    if (has_gatt && !has_hid && !peripheral && !name_looks_gamepad(name)) {
        return false;
    }
    if ((has_a2dp || audio_major || name_looks_audio(name)) && !has_hid && !name_looks_gamepad(name)) {
        return false;
    }
    if ((phone_major || computer_major) && !has_hid && !peripheral && !name_looks_gamepad(name)) {
        return false;
    }

    bool likely = has_hid || peripheral || name_looks_gamepad(name);
    if (out_likely) {
        *out_likely = likely;
    }

    if (likely) {
        return true;
    }
    /* Paired named HID-capable classic devices without clear class still show for manage */
    if (paired && name[0] != '\0' && !audio_major && !phone_major && !computer_major &&
        (strcasecmp(type, "bredr") == 0 || strcasecmp(type, "dual") == 0)) {
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
    char *raw = NULL;
    (void) luna_call(BT_ADAPTER "/setState", "{\"powered\":true,\"discoverable\":true}", &raw);
    free(raw);
    raw = NULL;
    bool ok = luna_call(BT_ADAPTER "/startDiscovery", "{}", &raw);
    free(raw);
    return ok;
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

static bool address_is_paired(const char *address) {
    array_list_t list;
    array_list_init(&list, sizeof(bt_gamepad_device_t), 8);
    bool ok = false;
    if (bt_gamepad_refresh_devices(&list)) {
        for (int i = 0, n = (int) array_list_size(&list); i < n; i++) {
            bt_gamepad_device_t *d = array_list_get(&list, i);
            if (strcasecmp(d->address, address) == 0 && d->paired) {
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
    if (err_buf && err_buf_len) {
        err_buf[0] = '\0';
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
                     "Это не похоже на геймпад. Подключение отменено.");
        }
        return false;
    }

    (void) bt_gamepad_stop_scan();
    /* Ensure adapter address known */
    if (!g_adapter[0]) {
        char *raw = NULL;
        (void) luna_call(BT_ADAPTER "/setState", "{\"powered\":true}", &raw);
        free(raw);
    }

    if (!paired) {
        char payload[192];
        json_with_adapter(payload, sizeof(payload), address, true);
        if (!luna_subscribe_wait(BT_ADAPTER "/pair", payload, "endPairing", 30, err_buf, err_buf_len)) {
            /* Some pads finish without endPairing keyword — accept if now paired */
            if (!address_is_paired(address)) {
                if (err_buf && err_buf_len && err_buf[0] == '\0') {
                    snprintf(err_buf, err_buf_len,
                             "Сопряжение не завершилось. Держите геймпад в режиме pairing и повторите.");
                }
                return false;
            }
        }
        usleep(500000);
    }

    char payload[192];
    json_with_adapter(payload, sizeof(payload), address, true);
    char hid_err[128] = {0};
    bool hid_ok = luna_subscribe_wait(BT_HID "/connect", payload, "\"connected\":true", 20,
                                      hid_err, sizeof(hid_err));
    if (!hid_ok) {
        /* Fallback one-shot */
        json_with_adapter(payload, sizeof(payload), address, false);
        char *raw = NULL;
        hid_ok = luna_call(BT_HID "/connect", payload, &raw);
        if (!hid_ok && raw && err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "HID connect: Failed to connect with remote device");
        }
        free(raw);
    }
    usleep(800000);
    if (!address_has_hid(address) && !hid_ok) {
        if (err_buf && err_buf_len) {
            if (hid_err[0]) {
                snprintf(err_buf, err_buf_len, "%s", hid_err);
            } else if (err_buf[0] == '\0') {
                snprintf(err_buf, err_buf_len,
                         "Не удалось открыть HID. Геймпад в pairing? Уже занят телефоном?");
            }
        }
        return false;
    }
    if (!address_has_hid(address)) {
        /* Connected flag without profile yet — soft success if hid_ok */
        if (hid_ok) {
            return true;
        }
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "Подключение прошло, но HID-профиль ещё не виден. Подождите и обновите список.");
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
    char payload[192];
    json_with_adapter(payload, sizeof(payload), address, false);
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
    char payload[192];
    json_with_adapter(payload, sizeof(payload), address, false);
    if (!luna_call(BT_ADAPTER "/unpair", payload, NULL)) {
        if (!luna_call(BT_DEVICE "/unpair", payload, NULL)) {
            if (err_buf && err_buf_len) {
                snprintf(err_buf, err_buf_len, "Не удалось забыть устройство");
            }
            return false;
        }
    }
    return true;
}
