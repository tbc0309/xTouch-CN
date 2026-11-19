#pragma once
inline BoardPowerProfile board_power_profile(bool sleeping){
 (void)sleeping;
 // Keep the network CPU clock stable across display sleep and wake.
 return {240,240,false};
}
inline void board_power_after_deep_sleep(){}
inline void board_power_before_deep_sleep(){}

inline bool board_touch_wake_prepare(){return XTOUCH_DEEP_TOUCH_PIN<0||esp_sleep_enable_ext0_wakeup((gpio_num_t)XTOUCH_DEEP_TOUCH_PIN,0)==ESP_OK;}
inline bool board_touch_woke(){return esp_sleep_get_wakeup_cause()==ESP_SLEEP_WAKEUP_EXT0;}
