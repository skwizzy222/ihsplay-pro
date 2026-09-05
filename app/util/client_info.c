#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "client_info.h"
#include "logging.h"

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
static void fill_random(uint8_t *buf, size_t len) {
    if (BCryptGenRandom(NULL, buf, (ULONG) len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        for (size_t i = 0; i < len; i++) {
            buf[i] = (uint8_t) (rand() & 0xFF);
        }
    }
}
#else
#include <fcntl.h>
#include <unistd.h>
static void fill_random(uint8_t *buf, size_t len) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, buf, len);
        close(fd);
        if (n == (ssize_t) len) {
            return;
        }
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t) (rand() & 0xFF);
    }
}
#endif

static bool load_identity_file(const char *path, uint64_t *device_id, uint8_t secret[32]) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return false;
    }
    uint8_t buf[40];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (n != sizeof(buf)) {
        return false;
    }
    memcpy(device_id, buf, 8);
    memcpy(secret, buf + 8, 32);
    return true;
}

static bool save_identity_file(const char *path, uint64_t device_id, const uint8_t secret[32]) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        return false;
    }
    uint8_t buf[40];
    memcpy(buf, &device_id, 8);
    memcpy(buf + 8, secret, 32);
    size_t n = fwrite(buf, 1, sizeof(buf), fp);
    fclose(fp);
    return n == sizeof(buf);
}

bool client_info_load_default(client_info_t *info) {
    memset(info, 0, sizeof(*info));
    info->config.deviceName = "IHSplay";

    char *pref = SDL_GetPrefPath("IHSplay", "ihsplay");
    char path[512];
    bool loaded = false;
    if (pref != NULL) {
        snprintf(path, sizeof(path), "%sdevice_identity.bin", pref);
        loaded = load_identity_file(path, &info->device_id, info->secret_key);
        if (!loaded) {
            fill_random((uint8_t *) &info->device_id, sizeof(info->device_id));
            fill_random(info->secret_key, sizeof(info->secret_key));
            if (!save_identity_file(path, info->device_id, info->secret_key)) {
                commons_log_warn("ClientInfo", "Failed to persist device identity to %s", path);
            } else {
                commons_log_info("ClientInfo", "Created new device identity");
            }
        } else {
            commons_log_info("ClientInfo", "Loaded persisted device identity");
        }
        SDL_free(pref);
    } else {
        fill_random((uint8_t *) &info->device_id, sizeof(info->device_id));
        fill_random(info->secret_key, sizeof(info->secret_key));
    }

    info->config.deviceId = info->device_id;
    info->config.secretKey = info->secret_key;
    return true;
}

void client_info_clear(client_info_t *info) {
    if (info->name != NULL) {
        free(info->name);
    }
    memset(info, 0, sizeof(*info));
}
