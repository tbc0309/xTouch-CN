#pragma once
#include "features/config.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#if XTOUCH_FEATURE_PIXEL_SCREEN
bool xtouch_pixel_display_get(bool *on);
bool xtouch_pixel_display_request(bool on);
void xtouch_pixel_display_poll(void);
#else
static inline bool xtouch_pixel_display_get(bool *on) {
    if (on)
        *on = true;
    return false;
}
static inline bool xtouch_pixel_display_request(bool on) {
    (void)on;
    return false;
}
static inline void xtouch_pixel_display_poll(void) {}
#endif
#ifdef __cplusplus
}
#endif
