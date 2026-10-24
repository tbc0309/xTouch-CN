#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Exact-fit labels manage padding from the complete string ink bounds. */
#define UI_TEXT_EXACT_BOUNDS LV_OBJ_FLAG_USER_2
/* Prefer a pre-generated native size; resample only unsupported/extreme fits and icons. */
void ui_text_scale(lv_obj_t *label, const lv_font_t *source, unsigned zoom);
bool ui_text_is_native(const lv_font_t *font);
const lv_font_t *ui_text_source(const lv_font_t *font);
void ui_text_guard(lv_obj_t *label);
void ui_text_guard_tree(lv_obj_t *root);
#ifdef __cplusplus
}
#endif
