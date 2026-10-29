#pragma once
#include <cstdio>
#include <esp_ota_ops.h>

// Call after local storage, display, touch and provisioning services are ready.
// Network availability is external to the candidate application's health.
inline esp_err_t xtouch_ota_confirm_local_startup() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (!running)
        return ESP_ERR_NOT_FOUND;
    esp_err_t error = esp_ota_get_state_partition(running, &state);
    if (error != ESP_OK)
        return error;
    if (state != ESP_OTA_IMG_NEW && state != ESP_OTA_IMG_PENDING_VERIFY)
        return ESP_OK;
    error = esp_ota_mark_app_valid_cancel_rollback();
    printf("[BOOT] OTA health confirmation: %s at 0x%lx\n", esp_err_to_name(error),
           (unsigned long)running->address);
    return error;
}
