#include "deep_sleep.h"
#include "features/config.h"
#include "features/sound/events.h"
#include "xtouch/power.h"
#include "xtouch/types.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "../../../boards/common/power_hooks.h"
static bool manualPending;
extern void xtouch_portal_release_transport();
void xtouch_deep_sleep_request(){manualPending=true;}
bool xtouch_deep_sleep_available(){return XTOUCH_FEATURE_DEEP_SLEEP;}
int xtouch_deep_sleep_touch_pin(){
 return XTOUCH_DEEP_TOUCH_PIN;
}
void xtouch_deep_sleep_boot(){
#if XTOUCH_FEATURE_DEEP_SLEEP
 board_power_after_deep_sleep();
#endif
}
void xtouch_deep_sleep_poll(bool safeIdle){
#if XTOUCH_FEATURE_DEEP_SLEEP
 static uint32_t eligibleAt;
 const bool eligible=(manualPending||xTouchConfig.xTouchDeepSleepAfter>0)&&xtouch_power_status().screenSleeping&&safeIdle&&!xtouch_sound_status().playing;
 if(!eligible){eligibleAt=0;manualPending=false;return;}
 if(!eligibleAt&&!manualPending){eligibleAt=millis();return;}
 if(!manualPending&&uint32_t(millis()-eligibleAt)<uint32_t(xTouchConfig.xTouchDeepSleepAfter)*60000u)return;
 int pin=xtouch_deep_sleep_touch_pin();
 if(pin>=0&&digitalRead(pin)==LOW)return; // Never sleep on an asserted wake source.
 // Deep sleep has no periodic main-core wake; only the board touch source remains.
 esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
 if(!board_touch_wake_prepare()){eligibleAt=millis();manualPending=false;return;}
 xtouch_sound_suspend(true);
 Serial.printf("[POWER] deep sleep, touchGPIO=%d\n",pin);
 Serial.flush();
 // Close the TLS/MQTT socket while the network interface is still alive.
 xtouch_portal_release_transport();
 WiFi.disconnect(true,false);
 board_power_before_deep_sleep();
 esp_deep_sleep_start();
#else
 (void)safeIdle;
#endif
}
