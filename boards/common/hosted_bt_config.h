#pragma once
#include "esp_hosted_bt_host_stack.h"

// Preserve the upstream C defaults with complete C++ zero initialization.
static inline esp_hosted_bt_host_stack_cfg_t xtouch_hosted_bt_config(bool bringUpController)
{
    esp_hosted_bt_host_stack_cfg_t cfg{};
    cfg.stack=ESP_HOSTED_BT_HOST_STACK_DEFAULT_TYPE;
    cfg.bring_up_controller=bringUpController;
    cfg.controller_ready_timeout_ms=EH_BT_CTRL_DEFAULT_READY_TIMEOUT_MS;
    return cfg;
}
