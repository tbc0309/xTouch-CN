#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
lv_obj_t *ui_detail_page_begin(void);
void ui_detail_page_end(bool restore);
void ui_detail_page_navigation(void);
void ui_detail_page_title(lv_obj_t *page,lv_obj_t *title);
void ui_detail_page_block(lv_obj_t *page,lv_obj_t *child,int y,int height);
void ui_detail_page_split(lv_obj_t *page,lv_obj_t *left,lv_obj_t *right,int y,int height,unsigned left_weight,unsigned right_weight);
#ifdef __cplusplus
}
#endif
