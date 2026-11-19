#pragma once
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
inline BoardPowerProfile board_power_profile(bool sleeping) {
    return {80, sleeping ? 160u : 240u, false};
}
inline void board_power_after_deep_sleep() {
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(GPIO_NUM_41);
    gpio_hold_dis(GPIO_NUM_1);
    rtc_gpio_deinit(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_1, 1);
}
inline void board_power_before_deep_sleep() {
    gpio_set_level(GPIO_NUM_41, 0);
    gpio_set_level(GPIO_NUM_1, 1);
    gpio_hold_en(GPIO_NUM_41);
    gpio_hold_en(GPIO_NUM_1);
    gpio_deep_sleep_hold_en();
}
inline bool board_touch_wake_prepare() {
    if (rtc_gpio_init(GPIO_NUM_0) != ESP_OK)
        return false;
    rtc_gpio_set_direction(GPIO_NUM_0, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(GPIO_NUM_0);
    auto result = esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    if (result != ESP_OK)
        rtc_gpio_deinit(GPIO_NUM_0);
    return result == ESP_OK;
}
// The BOOT key is the deep-sleep wake source; touch IRQ47 is not an RTC GPIO.
inline bool board_touch_woke() {
    return false;
}
