#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Keep explicit message lines separate; overflowing text scrolls horizontally without wrapping. */
void ui_dialog_text_layout(lv_obj_t *caption, int width);
#ifdef __cplusplus
}
#endif
