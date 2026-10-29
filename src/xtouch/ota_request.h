#pragma once
#include <stdbool.h>
#include "ui/lvgl_compat.h"
#ifdef __cplusplus
extern "C" {
#endif
void xtouch_ota_version_tap(lv_event_t *event);
bool xtouch_ota_take_request(void);
#ifdef __cplusplus
}
#endif
