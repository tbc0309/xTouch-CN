#include "errors.h"
#ifndef XTOUCH_ERROR_DETAILS
#define XTOUCH_ERROR_DETAILS 0
#endif
#include "bbl/error_actions.h"
#if XTOUCH_ERROR_DETAILS
#include "error_descriptions.h"
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if XTOUCH_ERROR_DETAILS
static const char *find_description(uint64_t code,
                                    const xtouch_error_description *rows, size_t count) {
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (rows[mid].code < code)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == count || rows[lo].code != code)
        return NULL;
    return rows[lo].en;
}
#endif

const char *xtouch_errors_getHMSErrorDescription(uint64_t code) {
#if XTOUCH_ERROR_DETAILS
    return find_description(code, hms_descriptions,
                            sizeof(hms_descriptions) / sizeof(*hms_descriptions));
#else
    (void)code; return NULL;
#endif
}

const char *xtouch_errors_getDeviceErrorDescription(uint32_t code) {
#if XTOUCH_ERROR_DETAILS
    return find_description(code, device_descriptions,
                            sizeof(device_descriptions) / sizeof(*device_descriptions));
#else
    (void)code; return NULL;
#endif
}

/* LVGL copies the current code into its label. No database or heap allocation. */
static char message[32];
static bool parse_key(const char *key, unsigned digits, uint64_t *value) {
    if (!key || strlen(key) != digits)
        return false;
    *value = 0;
    for (unsigned i = 0; i < digits; ++i) {
        unsigned char c = key[i];
        unsigned n;
        if (c >= '0' && c <= '9')
            n = c - '0';
        else if (c >= 'A' && c <= 'F')
            n = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            n = c - 'a' + 10;
        else
            return false;
        *value = (*value << 4) | n;
    }
    return *value != 0 && (digits != 16 || *value != UINT64_MAX);
}
static bool has_action(const char *key, const uint32_t *keys, unsigned count) {
    uint64_t value;
    if (!parse_key(key, 8, &value))
        return false;
    unsigned lo = 0, hi = count;
    while (lo < hi) {
        unsigned mid = lo + (hi - lo) / 2;
        if (keys[mid] < value)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < count && keys[lo] == value;
}
static bool universal_retry(const char *key) {
    return has_action(key, retry_codes, sizeof(retry_codes) / sizeof(*retry_codes));
}
unsigned xtouch_errors_deviceActions(const char *key, const char *serial) {
    uint64_t value;
    if (!parse_key(key, 8, &value))
        return 0;
    if (universal_retry(key))
        return 1;
    if (!serial || strlen(serial) < 3)
        return 0;
    for (unsigned d = 0; d < sizeof(error_action_devices) / sizeof(*error_action_devices); ++d) {
        if (strncmp(serial, error_action_devices[d], 3))
            continue;
        for (unsigned i = 0; i < sizeof(error_action_rows) / sizeof(*error_action_rows); ++i)
            if (error_action_rows[i].device == d && error_action_rows[i].code == value)
                return error_action_rows[i].flags;
        return 0;
    }
    return 0;
}
const char *xtouch_errors_getHMSError(const char *key) {
    uint64_t value;
    if (!parse_key(key, 16, &value))
        return NULL;
    snprintf(message, sizeof(message), "HMS %04X-%04X-%04X-%04X", (unsigned)(value >> 48),
             (unsigned)((value >> 32) & 65535), (unsigned)((value >> 16) & 65535),
             (unsigned)(value & 65535));
    return message;
}
const char *xtouch_errors_getDeviceError(const char *key) {
    uint64_t value;
    if (!parse_key(key, 8, &value))
        return NULL;
    snprintf(message, sizeof(message), "0x%08X", (unsigned)value);
    return message;
}
#ifdef XTOUCH_ERROR_SELFTEST
bool xtouch_errors_selftest(void) {
    uint64_t value;
    const char *hms_english=xtouch_errors_getHMSErrorDescription(UINT64_C(0x0500050000010007));
    const char *device_english=xtouch_errors_getDeviceErrorDescription(UINT32_C(0x07018004));
#if XTOUCH_ERROR_DETAILS
    bool descriptions_ok=hms_english&&device_english&&
                         !xtouch_errors_getDeviceErrorDescription(UINT32_C(0xFFFFFFFF));
#else
    bool descriptions_ok=!hms_english&&!device_english;
#endif
    return !strcmp(xtouch_errors_getHMSError("050005000001000f"), "HMS 0500-0500-0001-000F") &&
           !strcmp(xtouch_errors_getDeviceError("07018004"), "0x07018004") &&
           descriptions_ok &&
           parse_key("123456789ABCDEF0", 16, &value) && parse_key("12345678", 8, &value) &&
           !parse_key("GGGGGGGGGGGGGGGG", 16, &value) &&
           !parse_key("0000000000000000", 16, &value) &&
           !parse_key("FFFFFFFFFFFFFFFF", 16, &value) && !parse_key("00000000", 8, &value) &&
           !xtouch_errors_getDeviceError("123") && !xtouch_errors_getHMSError(NULL) &&
           (xtouch_errors_deviceActions("07018004", "093") & 1) &&
           (xtouch_errors_deviceActions("07FF8007", "093") & 2) &&
           !xtouch_errors_deviceActions("12345678", "093");
}
#endif
