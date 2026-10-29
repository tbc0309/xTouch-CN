#include "runtime.h"
#include "features/config.h"
#include "features/sound/events.h"
#include "platform/idf/runtime.h"
#include "platform/idf/wifi.h"
#include <esp_sleep.h>
#include "../../../boards/common/power_hooks.h"
#include <esp_pm.h>
static XTouchPowerStatus state={};
static esp_pm_lock_handle_t uiFrequencyLock;
static unsigned uiLockDepth;
void xtouch_power_ui_begin(){
 if(uiLockDepth){++uiLockDepth;return;}
 if(!uiFrequencyLock&&esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX,0,"ui-render",&uiFrequencyLock)!=ESP_OK)return;
 if(esp_pm_lock_acquire(uiFrequencyLock)==ESP_OK)uiLockDepth=1;
}
void xtouch_power_ui_end(){
 if(uiLockDepth&&!--uiLockDepth)esp_pm_lock_release(uiFrequencyLock);
}
void xtouch_power_screen(bool sleeping){
 state.screenSleeping=sleeping;xtouch_sound_suspend(sleeping);
 auto profile=board_power_profile(sleeping);
 esp_pm_config_t cfg={};cfg.min_freq_mhz=profile.minMHz;
 cfg.max_freq_mhz=profile.maxMHz;cfg.light_sleep_enable=profile.lightSleep;
 state.error=esp_pm_configure(&cfg);
 if(!state.error){state.automaticDfs=true;state.minMHz=cfg.min_freq_mhz;state.maxMHz=cfg.max_freq_mhz;state.automaticLightSleep=cfg.light_sleep_enable;}
 Serial.printf("[POWER] screen=%s cpu=%u..%u DFS=%d autoLightSleep=%d error=%d\n",sleeping?"sleep":"awake",state.minMHz,state.maxMHz,state.automaticDfs,state.automaticLightSleep,state.error);
}
void xtouch_power_begin(){xtouch_power_screen(false);WiFi.setSleep(true);}
bool xtouch_power_woke_from_touch(){return board_touch_woke();}
XTouchPowerStatus xtouch_power_status(){state.wakeCause=(uint32_t)esp_sleep_get_wakeup_cause();return state;}
