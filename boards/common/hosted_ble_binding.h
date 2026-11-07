#pragma once

// Pinned ESP-Hosted NimBLE glue. Release only the host HCI binding: controller
// lifecycle RPC is separate from the transport shared with Wi-Fi.
#include "esp_hosted_bt_host_stack_priv.h"

static inline esp_err_t xtouch_hosted_ble_bind() {
    return eh_bt_bind_nimble();
}

static inline void xtouch_hosted_ble_unbind() {
    eh_bt_unbind_nimble();
}
