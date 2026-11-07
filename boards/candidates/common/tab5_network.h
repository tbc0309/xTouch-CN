#pragma once
#include <bsp/esp-bsp.h>
#include <esp_hosted.h>
inline esp_err_t board_network_begin() {
    const auto result = bsp_feature_enable(BSP_FEATURE_WIFI, true);
    return result == ESP_OK ? esp_hosted_init() : result;
}
