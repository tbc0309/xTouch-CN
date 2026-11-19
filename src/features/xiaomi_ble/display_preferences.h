#pragma once
#include "features/config.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#if XTOUCH_FEATURE_XIAOMI_BLE
/* Mode IDs match the persisted environment preference: off/alternate/sensor. */
bool xtouch_environment_display_get(uint8_t *mode);
bool xtouch_environment_display_request(uint8_t mode);
void xtouch_environment_display_poll(void);
#else
static inline bool xtouch_environment_display_get(uint8_t *mode){if(mode)*mode=0;return false;}
static inline bool xtouch_environment_display_request(uint8_t mode){(void)mode;return false;}
static inline void xtouch_environment_display_poll(void){}
#endif
#ifdef __cplusplus
}
#endif
