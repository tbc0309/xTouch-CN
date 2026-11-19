#pragma once
#include "ui/lvgl_compat.h"
#ifndef XTOUCH_UI_LAYOUT
#define XTOUCH_UI_LAYOUT 0
#endif
#define UI_LAYOUT_LANDSCAPE 0
#define UI_LAYOUT_ROUND 1
#define UI_LAYOUT_SQUARE 2
typedef struct {int x,y,w,h,hit,gap;} ui_shape_area_t;
#ifdef __cplusplus
extern "C" {
#endif
ui_shape_area_t ui_shape_area(int width,int height,bool overlay);
/* Fixed geometry in physical pixels; never round-trip through design units. */
void ui_shape_place(lv_obj_t *object,int x,int y,int width,int height);
void ui_shape_screen(lv_obj_t *screen,int page);
void ui_shape_home_state(lv_obj_t *component);
void ui_shape_setup_choices(lv_obj_t *screen,lv_obj_t *title);
void ui_shape_dialog(lv_obj_t *component,bool error);
/* Physical-pixel typography roles: caption16, body18, title22, value24. */
void ui_shape_text(lv_obj_t *label,unsigned pixels);
void ui_shape_typography(lv_obj_t *root);
void ui_shape_theme(lv_obj_t *root);
typedef struct {lv_obj_t *row;unsigned group;} ui_shape_setting_t;
void ui_shape_settings(lv_obj_t *component,const ui_shape_setting_t *rows,unsigned count);
void ui_shape_keypad(lv_obj_t *component,lv_obj_t *input,lv_obj_t **tabs,unsigned count,lv_obj_t *more);
#ifdef __cplusplus
}
#endif
