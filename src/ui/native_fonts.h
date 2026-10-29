#pragma once
#include "ui/lvgl_compat.h"
#ifdef __cplusplus
extern "C" {
#endif
const lv_font_t *ui_native_font_select(const lv_font_t *source,unsigned zoom,const char *text);
#ifdef __cplusplus
}
#endif
