#pragma once
#include "ui/lvgl_compat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ui_scroll_guard ui_scroll_guard_t;
ui_scroll_guard_t *ui_scroll_guard_create(lv_obj_t *page);
void ui_scroll_guard_add(ui_scroll_guard_t *guard,lv_obj_t *control);
bool ui_scroll_busy(uint32_t quiet_ms);
// Separate page scrolling from control activation for pointer input.
void ui_settings_gesture_bind(lv_obj_t *page);
#ifdef __cplusplus
}
#endif
