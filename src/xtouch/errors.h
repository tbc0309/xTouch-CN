#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
unsigned xtouch_errors_deviceActions(const char *key,const char *serial);
/* Returned text remains valid until the next message lookup. */
const char *xtouch_errors_getHMSError(const char *key);
const char *xtouch_errors_getDeviceError(const char *key);
/* Description pointers refer to flash-resident catalog data. */
const char *xtouch_errors_getHMSErrorDescription(uint64_t code, bool chinese);
const char *xtouch_errors_getDeviceErrorDescription(uint32_t code, bool chinese);
#ifdef XTOUCH_ERROR_SELFTEST
bool xtouch_errors_selftest(void);
#endif
#ifdef __cplusplus
}
#endif
