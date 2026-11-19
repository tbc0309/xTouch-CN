#pragma once
inline BoardPowerProfile board_power_profile(bool sleeping){
 // DSI holds CPU_MAX/NO_LIGHT_SLEEP; rev 3.x supports 100/200/400 MHz.
 return {100,sleeping?200u:400u,false};
}
#if XTOUCH_FEATURE_DEEP_SLEEP
void board_touch_wake_restore();
void board_backlight_deep_sleep_restore();
void board_backlight_deep_sleep_hold();
bool board_touch_wake_prepare();
bool board_touch_woke();
inline void board_power_after_deep_sleep(){board_backlight_deep_sleep_restore();board_touch_wake_restore();}
inline void board_power_before_deep_sleep(){board_backlight_deep_sleep_hold();}
#else
inline bool board_touch_woke(){return false;}
inline void board_power_after_deep_sleep(){}
inline void board_power_before_deep_sleep(){}
#endif
