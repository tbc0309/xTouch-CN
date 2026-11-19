#pragma once
#include <esp_hosted.h>

inline esp_err_t board_network_begin() {
    return esp_hosted_init();
}
