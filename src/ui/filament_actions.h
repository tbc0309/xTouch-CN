#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
enum ui_filament_action { UI_FILAMENT_LOAD, UI_FILAMENT_UNLOAD, UI_FILAMENT_REREAD };
struct ui_filament_request { unsigned printer; int tray; unsigned action; };
bool xtouch_command_pending(void);
int ui_filament_selected(void);
bool ui_filament_select(int tray);
int ui_filament_temperature(int tray);
int ui_filament_current(int tray);
bool ui_filament_is_loaded(int tray);
int ui_filament_extruder(int tray);
bool ui_filament_allowed(unsigned action, int tray);
void ui_filament_report_error(uint32_t error);
void ui_filament_report_send_failed(void);
void ui_filament_report_rejection(uint32_t error, bool authorization);
void ui_filament_report_status(unsigned status);
void ui_filament_request_action(unsigned action);
void ui_filament_refresh(lv_obj_t *component);
void ui_filament_bind(lv_obj_t *component);
#ifdef __cplusplus
}
#endif
