#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
lv_obj_t *ui_settings_dropdown_row(lv_obj_t *parent,const char *title,lv_obj_t **caption,lv_obj_t **select);
lv_obj_t *ui_environment_setting_create(lv_obj_t *parent,lv_obj_t **caption,lv_obj_t **select);
#ifdef __cplusplus
}
#endif
