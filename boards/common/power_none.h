#pragma once
#include <sdkconfig.h>
inline BoardPowerProfile board_power_profile(bool) {
    return {CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, false};
}
inline void board_power_after_deep_sleep() {}
inline void board_power_before_deep_sleep() {}
inline bool board_touch_wake_prepare() {
    return false;
}
inline bool board_touch_woke() {
    return false;
}
