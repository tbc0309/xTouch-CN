#if XTOUCH_FEATURE_XIAOMI_BLE
#include "xiaomi_ble.h"
#endif
#include <Arduino.h>
#ifndef XTOUCH_WAVESHARE
// HTTP parsing, validated storage writes and SD mirroring share the loop task.
SET_LOOP_TASK_STACK_SIZE(12288);
#endif
#include <cmath>
using std::isfinite;
#include "xtouch/message_adapter.h"
#include <ArduinoJson.h>
#include "xtouch/debug.h"
#include "xtouch/paths.h"
#include "xtouch/storage.h"
#include "xtouch/hardware_settings.h"
#include "xtouch/types.h"
#include "xtouch/bblp.h"
#include "xtouch/globals.h"
#include "xtouch/filesystem.h"
#include "ui/ui.h"
#include "xtouch/sdcard.h"
#include "xtouch/hms.h"
#include "features/preview/print_preview.h"
#include "xtouch/power.h"
#include "features/power/deep_sleep.h"
#include "features/power/controller.h"
#include "features/power/idle_policy.h"
#if defined(XTOUCH_ENABLE_SCREEN_PROTECTION) && XTOUCH_ENABLE_SCREEN_PROTECTION
#include "features/power/screen_protection.h"
#include "ui/screen_protection.h"
#endif
#include "features/sound/events.h"
#include "features/diagnostics/loop_timing.h"
XTouchLoopTiming xtouch_loop_timing={};
#include "features/diagnostics/mqtt_timing.h"
XTouchMqttTiming xtouch_mqtt_timing={};
#include "features/indicator/indicator.h"
#include "../boards/common/network_hooks.h"

#if defined(XTOUCH_WAVESHARE)
#include "screen.h"
#elif defined(__XTOUCH_SCREEN_28__)
#include "devices/2.8/screen.h"
#endif

#include "xtouch/settings.h"
#include "xtouch/net.h"
#include "xtouch/firmware.h"
#include "xtouch/ota_request.h"
#include "xtouch/mqtt.h"
#include "xtouch/sensors/chamber.h"
#include "xtouch/events.h"
#include "xtouch/connection.h"
#include "xtouch/coldboot.h"
#include "xtouch/portal.h"
#ifdef XTOUCH_WAVESHARE
#include "camera.h"
#endif

static void xtouch_gui_poll(){xtouch_power_ui_begin();lv_timer_handler();xtouch_power_ui_end();}
#if defined(XTOUCH_ENABLE_SCREEN_PROTECTION) && XTOUCH_ENABLE_SCREEN_PROTECTION
static void xtouch_screen_protection_poll(){
 bool video=false;
#ifdef XTOUCH_WAVESHARE
 auto camera=xtouch_camera_status();video=camera.open&&camera.running;
#endif
 const auto p=xtouch_screen_protection(lv_disp_get_inactive_time(NULL),xTouchConfig.xTouchBacklightLevel,xtouch_screen_touchFromPowerOff,video,true);
 if(p.standby){xtouch_screen_sleep();return;}
 ui_screen_protection_offset(xTouchConfig.currentScreenIndex<0?0:p.x,xTouchConfig.currentScreenIndex<0?0:p.y);
 if(xtouch_screen_getBrightness()!=p.brightness)xtouch_screen_setBrightness(p.brightness);
}

#endif

void xtouch_portal_release_transport() {
  xtouch_pubSubClient.disconnect();xtouch_wiFiClientSecure.stop();
  xtouch_mqtt_hasStatus=false;xtouch_mqtt_nextAttempt=millis()+3000;
}

static bool runtimeConfigPending=false,runtimeProfilesPending=false;
void xtouch_runtime_config_reload(bool profiles){runtimeConfigPending=true;runtimeProfilesPending|=profiles;}
static void xtouch_runtime_config_apply(){
  if(!runtimeConfigPending||ui_scroll_busy(750))return;
  bool reconnect=false;
  if(runtimeProfilesPending&&!xtouch_profiles_reload(reconnect))return;
  runtimeConfigPending=runtimeProfilesPending=false;
  const auto previousLanguage=xtouch_language_get();
  const bool previousChamber=xTouchConfig.xTouchChamberSensorEnabled;
  const int page=xTouchConfig.currentScreenIndex;
  xtouch_settings_loadSettings(true);
  xtouch_clock_apply_settings();
  xtouch_screen_setScreenTimer((xTouchConfig.xTouchTFTOFFValue>0?xTouchConfig.xTouchTFTOFFValue:1)*60000U);
  if(xtouch_screen_onScreenOffTimer)lv_timer_reset(xtouch_screen_onScreenOffTimer);
  if(xtouch_screen_touchFromPowerOff)xtouch_screen_setBrightness(0);
  xtouch_sound_begin();
  if(previousChamber!=xTouchConfig.xTouchChamberSensorEnabled){if(xTouchConfig.xTouchChamberSensorEnabled)xtouch_chamber_timer_start();else xtouch_chamber_timer_stop();}
  if(reconnect){
#ifdef XTOUCH_WAVESHARE
    xtouch_camera_suspend();
#endif
    if(xtouch_printer_configured(xtouch_printer_active())){xtouch_pending_printer=xtouch_printer_active();xtouch_mqtt_switch_printer();}
    else{xtouch_portal_release_transport();xtouch_device_control_clear();memset(&bambuStatus,0,sizeof(bambuStatus));lastPrintState="";hms_reset();ui_printer_picker_close();loadScreen(0);}
  }
  if(previousLanguage!=xtouch_language_get()){
    ui_refresh_top_layer_language();
    loadScreen(reconnect?0:max(0,page));
  }else if(!reconnect&&page==4)loadScreen(4);
  Serial.printf("[CONFIG] applied without restart; reconnect=%d\n",int(reconnect));
}

void xtouch_intro_show(void)
{
  ui_introScreen_screen_init();
  lv_disp_load_scr(introScreen);
  xtouch_gui_poll();
}

void setup()
{

#if XTOUCH_USE_SERIAL == true || XTOUCH_DEBUG_ERROR == true || XTOUCH_DEBUG_DEBUG == true || XTOUCH_DEBUG_INFO == true
  Serial.begin(115200);
#endif

#ifdef XTOUCH_ERROR_SELFTEST
  Serial.printf("[ERROR-CODES] formatting, actions and faults: %s\n",(xtouch_errors_selftest()&&hms_selftest())?"PASS":"FAIL");
#endif
  ESP_ERROR_CHECK(board_network_begin());
  xtouch_deep_sleep_boot();
  bool storageReady = xtouch_storage_begin();
  xtouch_globals_init();
  if (storageReady) xtouch_settings_loadBootLanguage();
  xtouch_screen_setup();
  xtouch_intro_show();
  Serial.printf("[BOOT] display ready at %u ms\n",unsigned(millis()));
  while (!storageReady) {
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_STORAGE_ERROR);
    xtouch_gui_poll();
    delay(100);
  }
  xtouch_sdcard_setup();
  xtouch_storage_sync();
  // A recovery card containing only firmware.bin must not be blocked by
  // missing Wi-Fi/printer provisioning on an uninitialized device.
  if (!xtouch_storage_has(xtouch_paths_config)) xtouch_firmware_checkFirmwareUpdate();
  xtouch_coldboot_check();

  xtouch_settings_loadSettings();
  lv_label_set_text(introScreenCaption, XTOUCH_TEXT_STARTING);
  ui_refresh_top_layer_language();

  xtouch_firmware_checkFirmwareUpdate();

  xtouch_touch_setup();

  { DynamicJsonDocument bootConfig=xtouch_load_config(); }
  xtouch_portal_begin();
  while (WiFi.status() != WL_CONNECTED || xtouch_portal_provisioning()) {
    xtouch_gui_poll(); xtouch_portal_loop(); xtouch_sdcard_poll(false); delay(5);
  }

  Serial.printf("[BOOT] WiFi connected at %u ms\n",unsigned(millis()));
  // Non-blocking time synchronization also dates changes when OTA is disabled.
  xtouch_start_clock_sync();

  xtouch_firmware_checkOnlineFirmwareUpdate();

  xtouch_screen_setupScreenTimer();
  xtouch_setupGlobalEvents();

  xtouch_mqtt_setup();
  xtouch_chamber_timer_init();
  xtouch_power_begin();xtouch_sound_begin();xtouch_indicator_begin();
  xtouch_portal_boot_complete();
#if XTOUCH_FEATURE_XIAOMI_BLE
  xtouch_xiaomi_ble_begin();
#endif
  Serial.printf("[BOOT] application ready at %u ms\n",unsigned(millis()));
}

void loop()
{
  // Status changes invalidate LVGL and resume its refresh timer. Keep drawing
  // paused while sleeping, without stopping touch polling or network timers.
  if(xtouch_screen_touchFromPowerOff)lv_timer_pause(_lv_disp_get_refr_timer(lv_disp_get_default()));
  {XTouchLoopScope timing(XTOUCH_PHASE_GUI);xtouch_gui_poll();}
 #if defined(XTOUCH_ENABLE_SCREEN_PROTECTION) && XTOUCH_ENABLE_SCREEN_PROTECTION
  xtouch_screen_protection_poll();
 #endif
  // lv_task_handler is an alias of lv_timer_handler; run it only once.
  if (xtouch_language_apply_request()) {
    xtouch_settings_request_save();
    ui_refresh_top_layer_language();
    loadScreen(4);
  }
  if(xtouch_ota_take_request()) {
    // Check after returning from the touch event, with printer TLS buffers freed.
    xtouch_pubSubClient.disconnect();xtouch_wiFiClientSecure.stop();xtouch_mqtt_hasStatus=false;
    loadScreen(-1);xtouch_firmware_checkOnlineFirmwareUpdate(true);
    uint32_t shown=millis();while(millis()-shown<1800){xtouch_gui_poll();delay(5);}
    loadScreen(4);xtouch_mqtt_nextAttempt=millis();
  }
  {XTouchLoopScope timing(XTOUCH_PHASE_PORTAL);xtouch_portal_loop();}
  xtouch_runtime_config_apply();
  xtouch_settings_poll();
#ifdef XTOUCH_WAVESHARE
  xtouch_camera_poll();
#endif
  xtouch_device_filament_poll();
  if (xtouch_portal_maintenance()) {
    if (xtouch_pubSubClient.connected()) xtouch_pubSubClient.disconnect();
    xtouch_wiFiClientSecure.stop();
    if (xtouch_cloud_job_pending()) xtouch_cloud_job_result();
  } else {XTouchLoopScope timing(XTOUCH_PHASE_MQTT);xtouch_mqtt_loop();}
  {XTouchLoopScope timing(XTOUCH_PHASE_PREVIEW);xtouch_print_preview_poll();}
  static bool clockReady = false;
  if (!clockReady && time(NULL) > 1577836800 && lv_disp_get_inactive_time(NULL) >= 3000 && !ui_scroll_busy(3000)) {
    xtouch_storage_clock_ready();
    xtouch_storage_sync(true);
    clockReady = true;
  }
  #ifdef XTOUCH_WAVESHARE
  if(!xtouch_camera_status().open)
#endif
  {XTouchLoopScope timing(XTOUCH_PHASE_SD);xtouch_sdcard_poll();}
  if (xtouch_storage_restart_required() || xtouch_sdcard_firmware_waiting) {
    // Imported configuration is applied as one consistent boot snapshot.
    if (xtouch_settings_flush()) { delay(200); ESP.restart(); }
  }
  static bool printingPreviously=false;
  static unsigned printingOwner=~0u;
  bool printingNow=bambuStatus.print_status==XTOUCH_PRINT_STATUS_RUNNING;
  if(printingPreviously&&printingOwner==xtouch_printer_active()&&bambuStatus.print_status==XTOUCH_PRINT_STATUS_FINISHED)
    xtouch_sound_event(XTOUCH_SOUND_COMPLETE,millis());
  if(xtouch_printer_online())printingPreviously=printingNow||bambuStatus.print_status==XTOUCH_PRINT_STATUS_PAUSED;
  printingOwner=xtouch_printer_active();
  xtouch_indicator_poll(xtouch_printer_online(),hms_print_error()!=0,xtouch_screen_touchFromPowerOff,xTouchConfig.xTouchStatusLed);
  xtouch_display_poll(xtouch_deep_idle_reported(lastPrintState.c_str())&&xtouch_deep_idle_allowed(bambuStatus.print_status,xtouch_printer_online(),hms_print_error()!=0,xtouch_portal_maintenance(),xtouch_print_preview_status().busy));

  delay(xtouch_screen_touchFromPowerOff?20:2);

}
