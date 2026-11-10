#pragma once
#include <driver/rtc_io.h>
#include <esp_sleep.h>
inline BoardPowerProfile board_power_profile(bool sleeping) {
    return {80, sleeping ? 160u : 240u, false};
}
inline void board_power_after_deep_sleep() {
    rtc_gpio_deinit(GPIO_NUM_0);
}
inline void board_power_before_deep_sleep() {}
inline bool board_touch_wake_prepare() {
    if (rtc_gpio_init(GPIO_NUM_0) != ESP_OK)
        return false;
    rtc_gpio_set_direction(GPIO_NUM_0, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(GPIO_NUM_0);
    auto e = esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    if (e != ESP_OK)
        rtc_gpio_deinit(GPIO_NUM_0);
    return e == ESP_OK;
}
inline bool board_touch_woke() {
    return false;
}
