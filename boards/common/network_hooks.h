#pragma once
#include <esp_err.h>
#if defined(XTOUCH_BOARD_P4)
#include "../esp32-p4-wifi6-touch-lcd-4.3/network.h"
#else
inline esp_err_t board_network_begin() { return ESP_OK; }
#endif
