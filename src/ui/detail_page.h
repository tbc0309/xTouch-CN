#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
lv_obj_t *ui_detail_page_begin(void);
void ui_detail_page_end(bool restore);
void ui_detail_page_navigation(void);
#ifdef __cplusplus
}
#endif
