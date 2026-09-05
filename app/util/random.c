#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "random.h"

/* Prefer crypto-quality entropy when available via rand_s / /dev/urandom fallbacks are platform-specific;
 * SDL or mbedtls may not be linked into this translation unit, so use a mixed approach. */

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

static uint32_t secure_u32(void) {
    uint32_t value = 0;
    if (BCryptGenRandom(NULL, (PUCHAR) &value, sizeof(value), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        value = (uint32_t) (GetTickCount() ^ GetCurrentProcessId());
    }
    return value;
}
#else
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

static uint32_t secure_u32(void) {
    uint32_t value = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, &value, sizeof(value));
        close(fd);
        if (n == (ssize_t) sizeof(value)) {
            return value;
        }
    }
    value = (uint32_t) time(NULL);
    value ^= (uint32_t) (uintptr_t) &value;
    return value;
}
#endif

void random_pin(char *pin) {
    uint32_t value = secure_u32() % 10000u;
    sprintf(pin, "%04u", value);
}
