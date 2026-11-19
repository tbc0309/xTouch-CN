#pragma once
#include "storage.h"

void xtouch_hardware_set_flip(uint8_t value) {
    StaticJsonDocument<64> doc; doc["flip"] = value ? 1 : 0;
    xtouch_storage_write("/xtouch/hardware.json", doc);
}
uint8_t xtouch_hardware_get_flip() {
    auto doc = xtouch_storage_read("/xtouch/hardware.json", 128);
    return doc["flip"] | 0;
}
