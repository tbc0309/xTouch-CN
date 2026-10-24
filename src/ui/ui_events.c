#include "nozzle_details.h"
#include "filament_actions.h"
#include "ui.h"
#include "../../src/xtouch/types.h"

void initialActions(lv_event_t *e) {}

/* -----------Sidebar-------------- */
void onSidebarHome(lv_event_t *e) { loadScreen(0); }
void onSidebarTemp(lv_event_t *e) { loadScreen(1); }
void onSidebarControl(lv_event_t *e) { loadScreen(2); }
void onSidebarNozzle(lv_event_t *e) { loadScreen(3); }
void onSidebarSettings(lv_event_t *e) { loadScreen(4); }

/* -----------Home-------------- */
void onHomeControllerPlayPauseConfirm()
{
    if(xTouchConfig.currentScreenIndex!=0||!xtouch_printer_online())return;
    lv_obj_t *target = ui_comp_get_child(ui_homeComponent, UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER_MAINSCREENPLAYPAUSEBUTTON);
    lv_msg_send(XTOUCH_COMMAND_PAUSE, NULL);
    lv_obj_add_state(target, LV_STATE_DISABLED);
}
void onHomeControllerPlayPause(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    switch (bambuStatus.print_status)
    {
    case XTOUCH_PRINT_STATUS_PAUSED:
        lv_msg_send(XTOUCH_COMMAND_RESUME, NULL);
        lv_obj_add_state(target, LV_STATE_DISABLED);
        break;
    case XTOUCH_PRINT_STATUS_RUNNING:
    case XTOUCH_PRINT_STATUS_PREPARE:
        ui_confirmPanel_show(XTOUCH_TEXT_PAUSE_PRINT, onHomeControllerPlayPauseConfirm);
        break;
    }

    // cui_mainScreenPlayPauseButton
}

void onHomeControllerStopConfirm() { lv_msg_send(XTOUCH_COMMAND_STOP, NULL); }
void onHomeControllerStop(lv_event_t *e)
{
    ui_confirmPanel_show_danger(XTOUCH_TEXT_CANCEL_PRINT, onHomeControllerStopConfirm);
}
void onHomeSpeedSelection(lv_event_t *e) {}
void onHomeLight(lv_event_t *e) { lv_msg_send(XTOUCH_COMMAND_LIGHT_TOGGLE, NULL); }
void onHomeBedTemp(lv_event_t *e)
{
    loadScreen(1);

    ui_temperatureComponent_show_keypad_and_back(0, 0, 0);

    lv_obj_t *target = ui_comp_get_child(ui_temperatureComponent, UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTTEMPS_TEMPERATURECOMPONENTBED);
    lv_obj_add_state(target, LV_STATE_CHECKED);
}
void onHomeNozzleTemp(lv_event_t *e, int backToScreen)
{
    if(bambuStatus.nozzle_count){ui_nozzle_details_show();return;}
    loadScreen(1);
    ui_temperatureComponent_show_keypad_and_back(0, 1, backToScreen);

    lv_obj_t *target = ui_comp_get_child(ui_temperatureComponent, UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTTEMPS_TEMPERATURECOMPONENTNOZZLE);
    lv_obj_add_state(target, LV_STATE_CHECKED);
}

/* -----------Temps-------------- */
void onTemperatureBed(const char *value)
{
    LV_LOG_USER("onTemperatureBed: %s", value);

    lv_msg_send(XTOUCH_COMMAND_BED_TARGET_TEMP, value);
}
void onTemperatureNozzle(int32_t value) { lv_msg_send(XTOUCH_COMMAND_NOZZLE_TARGET_TEMP, &value); }
void onTemperaturePart(lv_event_t *e) { lv_msg_send(XTOUCH_CONTROL_INC_SWITCH, NULL); }
void onTemperatureAux(lv_event_t *e) { lv_msg_send(XTOUCH_CONTROL_INC_SWITCH, NULL); }
void onTemperatureKeypad(lv_event_t *e) { lv_msg_send(XTOUCH_CONTROL_INC_SWITCH, NULL); }

/* -----------Control-------------- */
void onControlRange(lv_event_t *e) { lv_msg_send(XTOUCH_CONTROL_INC_SWITCH, NULL); }
bool ui_manual_motion_allowed(void){return !xtouch_command_pending()&&xtouch_printer_online()&&!xtouch_printer_switch_pending()&&!bambuStatus.non_fdm_mode&&!bambuStatus.extruder_busy&&(bambuStatus.print_status==XTOUCH_PRINT_STATUS_IDLE||bambuStatus.print_status==XTOUCH_PRINT_STATUS_FINISHED||bambuStatus.print_status==XTOUCH_PRINT_STATUS_FAILED);}
static void home_confirm(void){if(ui_manual_motion_allowed())lv_msg_send(XTOUCH_COMMAND_HOME,NULL);}
void onControlHome(lv_event_t *e){if(ui_manual_motion_allowed())ui_confirmPanel_show(xtouch_local("归位？打印头将移动", "Home axes? Toolhead will move."),home_confirm);}
void onControlLeft(lv_event_t *e) { if(ui_manual_motion_allowed())lv_msg_send(XTOUCH_COMMAND_LEFT, NULL); }
void onControlRight(lv_event_t *e) { if(ui_manual_motion_allowed())lv_msg_send(XTOUCH_COMMAND_RIGHT, NULL); }
void onControlUp(lv_event_t *e) { if(ui_manual_motion_allowed())lv_msg_send(XTOUCH_COMMAND_UP, NULL); }
void onControlDown(lv_event_t *e) { if(ui_manual_motion_allowed())lv_msg_send(XTOUCH_COMMAND_DOWN, NULL); }
void onControlAxis(lv_event_t *e) { lv_msg_send(XTOUCH_CONTROL_AXIS_SWITCH, NULL); }

/* Settings */

void onSettingsResetDeviceConfirm() { lv_msg_send(XTOUCH_SETTINGS_RESET_DEVICE, NULL); }
void onSettingsResetDevice(lv_event_t *e)
{
    ui_confirmPanel_show(XTOUCH_TEXT_REBOOT, onSettingsResetDeviceConfirm);
}

void onSettingsWOP(lv_event_t *e)
{
    xTouchConfig.xTouchWakeOnPrint = !xTouchConfig.xTouchWakeOnPrint;
    lv_msg_send(XTOUCH_SETTINGS_SAVE, NULL);
}

void onSettingsAuxFan(lv_event_t *e)
{
    xTouchConfig.xTouchAuxFanEnabled = !xTouchConfig.xTouchAuxFanEnabled;
    lv_msg_send(XTOUCH_SETTINGS_SAVE, NULL);
}

void onSettingsChamberFan(lv_event_t *e)
{
    xTouchConfig.xTouchChamberFanEnabled = !xTouchConfig.xTouchChamberFanEnabled;
    lv_msg_send(XTOUCH_SETTINGS_SAVE, NULL);
}

void onSettingsChamberSensor(lv_event_t *e)
{
    xTouchConfig.xTouchChamberSensorEnabled = !xTouchConfig.xTouchChamberSensorEnabled;
    lv_msg_send(XTOUCH_SETTINGS_CHAMBER_TEMP, NULL);
    lv_msg_send(XTOUCH_SETTINGS_SAVE, NULL);
}

void onSettingsTFTFlipConfirm()
{
    lv_msg_send(XTOUCH_SETTINGS_TFT_FLIP, NULL);
}

void onSettingsOTA(lv_event_t *e)
{
    xTouchConfig.xTouchOTAEnabled = !xTouchConfig.xTouchOTAEnabled;
    lv_msg_send(XTOUCH_SETTINGS_SAVE, NULL);
}

void onSettingsTFTFlip(lv_event_t *e)
{
    ui_confirmPanel_show(XTOUCH_TEXT_FLIP_CONFIRM, onSettingsTFTFlipConfirm);
}

/* filament */
void onNozzleUp(lv_event_t *e)
{
    if(bambuStatus.nozzle_count){ui_nozzle_details_show();return;}
    lv_msg_send(XTOUCH_COMMAND_EXTRUDE_UP, NULL);
}

void onNozzleDown(lv_event_t *e)
{
    if(bambuStatus.nozzle_count){ui_nozzle_details_show();return;}
    lv_msg_send(XTOUCH_COMMAND_EXTRUDE_DOWN, NULL);
    
}

void onFilamentUnload(lv_event_t *e) { ui_filament_request_action(UI_FILAMENT_UNLOAD); }
void onFilamentLoad(lv_event_t *e) { ui_filament_request_action(UI_FILAMENT_LOAD); }
