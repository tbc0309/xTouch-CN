#pragma once
#include <esp_err.h>
#if defined(XTOUCH_CANDIDATE_TAB5)
#include <bsp/esp-bsp.h>
#include <esp_hosted.h>
inline esp_err_t board_network_begin() {
    const auto result = bsp_feature_enable(BSP_FEATURE_WIFI, true);
    return result == ESP_OK ? esp_hosted_init() : result;
}
#elif defined(XTOUCH_BOARD_P4)
#include "../esp32-p4-wifi6-touch-lcd-4.3/network.h"
#else
inline esp_err_t board_network_begin() { return ESP_OK; }
#endif
