#include <Arduino.h>
#include <Wire.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "ulp_lp_core.h"
#include "xtouch_lp_touch.h"

extern const uint8_t touchBinaryStart[] asm("_binary_xtouch_lp_touch_bin_start");
extern const uint8_t touchBinaryEnd[] asm("_binary_xtouch_lp_touch_bin_end");
static bool wokeByTouch;

void board_touch_wake_restore(){
    wokeByTouch=esp_sleep_get_wakeup_cause()==ESP_SLEEP_WAKEUP_ULP&&ulp_touch_fired==1;
    ulp_lp_core_stop();
    rtc_gpio_deinit(GPIO_NUM_7);rtc_gpio_deinit(GPIO_NUM_8);
}
bool board_touch_woke(){return wokeByTouch;}
bool board_touch_wake_prepare(){
    uint8_t address=0;
    for(uint8_t candidate:{uint8_t(0x5d),uint8_t(0x14)}){
        Wire.beginTransmission(candidate);if(!Wire.endTransmission()){address=candidate;break;}
    }
    if(!address)return false;
    Wire.end();
    bool ok=rtc_gpio_init(GPIO_NUM_7)==ESP_OK&&rtc_gpio_init(GPIO_NUM_8)==ESP_OK;
    if(ok)ok=ulp_lp_core_load_binary(touchBinaryStart,touchBinaryEnd-touchBinaryStart)==ESP_OK;
    if(ok){
        ulp_touch_address=address;
        ulp_lp_core_cfg_t config={};config.wakeup_source=ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER;
        config.lp_timer_sleep_duration_us=200000;
        ok=esp_sleep_enable_ulp_wakeup()==ESP_OK&&ulp_lp_core_run(&config)==ESP_OK;
    }
    if(ok){
        uint32_t began=millis();while(!ulp_touch_samples&&uint32_t(millis()-began)<500)delay(1);
        ok=ulp_touch_samples&&ulp_touch_ok;
    }
    if(ok){Serial.println("[POWER] LP touch ready, poll=200 ms");return true;}
    ulp_lp_core_stop();esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ULP);
    rtc_gpio_deinit(GPIO_NUM_7);rtc_gpio_deinit(GPIO_NUM_8);
    Wire.begin(7,8,100000);Wire.setTimeOut(20);
    Serial.println("[POWER] LP touch setup failed; keep network standby");
    return false;
}
