#pragma once
#include "lvgl_compat.h"

/* Shared native-pixel component tokens. Geometry scales with the canvas;
 * text retains its semantic font tier and never shrinks to fit a caption. */
typedef struct {
    int unit, gap, inset, radius, control, icon;
} ui_specs_t;
typedef struct {
    lv_area_t swatch;
    lv_area_t captions[2];
} ui_material_content_t;

#ifdef __cplusplus
extern "C" {
#endif
ui_specs_t ui_specs_get(void);
void ui_specs_fill_remaining(lv_obj_t *item, bool vertical);
void ui_specs_text_window(lv_obj_t *label, int width);
void ui_layout_action_fit(lv_obj_t *button, int width, int height, int pad, int gap);
ui_material_content_t ui_specs_material_content(lv_area_t card);
#ifdef __cplusplus
}
#endif
