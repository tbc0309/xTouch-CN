#pragma once
#include "features/config.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#if XTOUCH_FEATURE_GCODE_BROWSER
bool xtouch_files_available(void);
bool xtouch_files_network_busy(void);
void xtouch_files_open(void);
void xtouch_files_poll(void);
void xtouch_files_control_reply(const char *sequence, const char *command, const char *result,
                                const char *reason, unsigned error);
#else
static inline bool xtouch_files_network_busy(void) {
    return false;
}
static inline bool xtouch_files_available(void) {
    return false;
}
static inline void xtouch_files_open(void) {}
static inline void xtouch_files_poll(void) {}
static inline void xtouch_files_control_reply(const char *s, const char *c, const char *r,
                                              const char *why, unsigned e) {
    (void)s;
    (void)c;
    (void)r;
    (void)why;
    (void)e;
}
#endif
#ifdef __cplusplus
}
#endif
