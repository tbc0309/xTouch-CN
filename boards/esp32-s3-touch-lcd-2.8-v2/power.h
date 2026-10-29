#pragma once
#include "platform/idf/runtime.h"
#include <driver/gpio.h>
inline BoardPowerProfile board_power_profile(bool sleeping){
 // Keep the Wi-Fi HTTP server reachable in network standby.
 // Avoid the reported intermittent HTTP loss with automatic light sleep.
 return {80,sleeping?160u:240u,false};
}
inline void board_power_after_deep_sleep(){
 pinMode(7,OUTPUT);digitalWrite(7,HIGH);
 gpio_deep_sleep_hold_dis();gpio_hold_dis(GPIO_NUM_7);
}
inline void board_power_before_deep_sleep(){
 gpio_hold_en(GPIO_NUM_7);gpio_deep_sleep_hold_en();
}

inline bool board_touch_wake_prepare(){return XTOUCH_DEEP_TOUCH_PIN<0||esp_sleep_enable_ext0_wakeup((gpio_num_t)XTOUCH_DEEP_TOUCH_PIN,0)==ESP_OK;}
inline bool board_touch_woke(){return esp_sleep_get_wakeup_cause()==ESP_SLEEP_WAKEUP_EXT0;}
