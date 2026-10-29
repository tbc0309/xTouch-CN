#pragma once
#include "platform/idf/runtime.h"
#include "port_profile.h"
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
inline BoardPowerProfile board_power_profile(bool sleeping) {
#if CONFIG_IDF_TARGET_ESP32P4
    return {100, sleeping ? 200u : (CONFIG_ESP32P4_REV_MIN_FULL >= 300 ? 400u : 360u), false};
#else
    return {80, sleeping ? 160u : 240u, false};
#endif
}
inline void board_power_after_deep_sleep() {
#if !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
    gpio_deep_sleep_hold_dis();
#endif
    const int backlight = XTouchPort::profile().backlight;
    if (backlight >= 0)
        gpio_hold_dis(gpio_num_t(backlight));
    if (XTOUCH_DEEP_WAKE_PIN >= 0 && rtc_gpio_is_valid_gpio(gpio_num_t(XTOUCH_DEEP_WAKE_PIN)))
        rtc_gpio_deinit(gpio_num_t(XTOUCH_DEEP_WAKE_PIN));
}
inline void board_power_before_deep_sleep() {
    const int backlight = XTouchPort::profile().backlight;
    if (backlight >= 0) {
        gpio_hold_en(gpio_num_t(backlight));
#if !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
        gpio_deep_sleep_hold_en();
#endif
    }
}
inline bool board_touch_wake_prepare() {
#if XTOUCH_DEEP_WAKE_PIN < 0
    return false;
#else
    const gpio_num_t pin = gpio_num_t(XTOUCH_DEEP_WAKE_PIN);
    if (pin < 0 || !rtc_gpio_is_valid_gpio(pin) || rtc_gpio_init(pin) != ESP_OK)
        return false;
    if (rtc_gpio_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY) != ESP_OK ||
        rtc_gpio_pulldown_dis(pin) != ESP_OK || rtc_gpio_pullup_en(pin) != ESP_OK) {
        rtc_gpio_deinit(pin);
        return false;
    }
#if CONFIG_IDF_TARGET_ESP32P4
    const auto result = esp_sleep_enable_ext1_wakeup_io(1ULL << pin, ESP_EXT1_WAKEUP_ANY_LOW);
#else
    const auto result = esp_sleep_enable_ext0_wakeup(pin, 0);
#endif
    if (result != ESP_OK)
        rtc_gpio_deinit(pin);
    return result == ESP_OK;
#endif
}
inline bool board_touch_woke() {
    return false;
}
