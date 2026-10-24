#pragma once
#include <ArduinoJson.h>
void xtouch_xiaomi_ble_begin();
void xtouch_xiaomi_ble_status(JsonObject out);
bool xtouch_xiaomi_ble_bind(const char *mac,const char *key);

bool xtouch_xiaomi_ble_key_required();

void xtouch_xiaomi_ble_model_status(JsonObject out);
