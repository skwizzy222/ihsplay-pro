#define _GNU_SOURCE

#include "backend/bt_gamepad_manager.h"

#include "logging.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

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

static bool device_looks_interesting(jvalue_ref device) {
    jvalue_ref name = jobject_get(device, j_cstr_to_buffer("name"));
    jvalue_ref type = jobject_get(device, j_cstr_to_buffer("typeOfDevice"));
    jvalue_ref paired = jobject_get(device, j_cstr_to_buffer("paired"));
    jvalue_ref connected = jobject_get(device, j_cstr_to_buffer("connected"));
    jvalue_ref profiles = jobject_get(device, j_cstr_to_buffer("connectedProfiles"));

    if (jis_boolean(paired)) {
        bool v = false;
        jboolean_get(paired, &v);
        if (v) {
            return true;
        }
    }
    if (jis_boolean(connected)) {
        bool v = false;
        jboolean_get(connected, &v);
        if (v) {
            return true;
        }
    }
    if (jis_array(profiles)) {
        return true;
    }
    if (jis_string(type)) {
        raw_buffer t = jstring_get(type);
        if (t.m_str && (strcasestr(t.m_str, "hid") || strcasestr(t.m_str, "peripheral") ||
                        strcasestr(t.m_str, "joystick") || strcasestr(t.m_str, "gamepad") ||
                        strcasestr(t.m_str, "keyboard") || strcasestr(t.m_str, "mouse"))) {
            return true;
        }
    }
    if (jis_string(name)) {
        raw_buffer n = jstring_get(name);
        if (n.m_len > 0) {
            return true;
        }
    }
    return false;
}

static void parse_device(jvalue_ref device, bt_gamepad_device_t *out) {
    memset(out, 0, sizeof(*out));
    jvalue_ref address = jobject_get(device, j_cstr_to_buffer("address"));
    jvalue_ref name = jobject_get(device, j_cstr_to_buffer("name"));
    jvalue_ref paired = jobject_get(device, j_cstr_to_buffer("paired"));
    jvalue_ref connected = jobject_get(device, j_cstr_to_buffer("connected"));
    jvalue_ref profiles = jobject_get(device, j_cstr_to_buffer("connectedProfiles"));

    if (jis_string(address)) {
        raw_buffer a = jstring_get(address);
        snprintf(out->address, sizeof(out->address), "%.*s", (int) a.m_len, a.m_str);
    }
    if (jis_string(name) && jstring_get(name).m_len > 0) {
        raw_buffer n = jstring_get(name);
        snprintf(out->name, sizeof(out->name), "%.*s", (int) n.m_len, n.m_str);
    } else {
        snprintf(out->name, sizeof(out->name), "%s", out->address[0] ? out->address : "Bluetooth");
    }
    if (jis_boolean(paired)) {
        bool v = false;
        jboolean_get(paired, &v);
        out->paired = v;
    }
    if (jis_boolean(connected)) {
        bool v = false;
        jboolean_get(connected, &v);
        out->connected = v;
    }
    if (jis_array(profiles)) {
        for (ssize_t i = 0; i < jarray_size(profiles); i++) {
            jvalue_ref p = jarray_get(profiles, i);
            if (!jis_string(p)) {
                continue;
            }
            raw_buffer pb = jstring_get(p);
            if (pb.m_str && strcasestr(pb.m_str, "hid")) {
                out->connected_hid = true;
                out->connected = true;
                break;
            }
        }
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
            if (!jis_object(d) || !device_looks_interesting(d)) {
                continue;
            }
            bt_gamepad_device_t parsed;
            parse_device(d, &parsed);
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
    if (bt_gamepad_refresh_devices(&list)) {
        for (int i = 0, n = (int) array_list_size(&list); i < n; i++) {
            bt_gamepad_device_t *d = array_list_get(&list, i);
            if (strcasecmp(d->address, address) == 0) {
                paired = d->paired;
                if (d->connected_hid) {
                    array_list_deinit(&list);
                    return true;
                }
                break;
            }
        }
    }
    array_list_deinit(&list);

    (void) bt_gamepad_stop_scan();

    if (!paired) {
        char payload[160];
        snprintf(payload, sizeof(payload), "{\"address\":\"%s\",\"subscribe\":true}", address);
        char *raw = NULL;
        if (!luna_call(BT_ADAPTER "/pair", payload, &raw)) {
            commons_log_warn("BTGamepad", "Pair call failed for %s, trying HID anyway", address);
            if (err_buf && err_buf_len && raw) {
                snprintf(err_buf, err_buf_len, "Сопряжение не подтверждено, пробуем connect…");
            }
        }
        free(raw);
        /* Give firmware time to finish bonding after first pair reply */
        usleep(2500000);
    }

    char payload[128];
    snprintf(payload, sizeof(payload), "{\"address\":\"%s\"}", address);
    char *raw = NULL;
    bool ok = luna_call(BT_HID "/connect", payload, &raw);
    if (!ok) {
        if (err_buf && err_buf_len) {
            snprintf(err_buf, err_buf_len, "HID connect не удался");
        }
        free(raw);
        return false;
    }
    free(raw);
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
            snprintf(err_buf, err_buf_len, "Не удалось отключить HID");
        }
        return false;
    }
    return true;
}
