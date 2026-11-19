#pragma once
#include "ui/lvgl_compat.h"
#ifdef __cplusplus
extern "C" {
#endif
/* UI-thread only. The image owner retains the pixels until cleared. */
void ui_home_preview_set(const lv_img_dsc_t *image);
void ui_home_preview_bounds(uint16_t *width,uint16_t *height);
#ifdef __cplusplus
}
#endif
