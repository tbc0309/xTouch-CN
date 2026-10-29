#pragma once

// Deep sleep stays disabled until each board's independent wake input is
// mapped and validated. Ordinary display standby keeps touch polling alive.
inline BoardPowerProfile board_power_profile(bool sleeping) {
#if CONFIG_IDF_TARGET_ESP32P4
    return {100, sleeping ? 200u : (CONFIG_ESP32P4_REV_MIN_FULL >= 300 ? 400u : 360u), false};
#else
    return {80, sleeping ? 160u : 240u, false};
#endif
}
inline void board_power_after_deep_sleep() {}
inline void board_power_before_deep_sleep() {}
inline bool board_touch_wake_prepare() { return false; }
inline bool board_touch_woke() { return false; }
