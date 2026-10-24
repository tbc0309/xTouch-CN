#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
unsigned ui_ams_base(void);
unsigned ui_ams_selected_unit(void);
bool ui_ams_drying_available(void);
bool ui_ams_select_unit(unsigned unit);
uint32_t ui_ams_slot_color(unsigned slot);
uint32_t ui_ams_remaining_color(unsigned percent);
void ui_ams_strip_bind(lv_obj_t *strip);
void ui_ams_filament_layout(lv_obj_t *component);
#ifdef __cplusplus
}
#endif
