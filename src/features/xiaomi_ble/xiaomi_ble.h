#pragma once
#include <ArduinoJson.h>
void xtouch_xiaomi_ble_begin();
void xtouch_xiaomi_ble_status(JsonObject out);
bool xtouch_xiaomi_ble_bind(const char *mac,const char *key);

bool xtouch_xiaomi_ble_key_required();

void xtouch_xiaomi_ble_model_status(JsonObject out);

#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION && CONFIG_IDF_TARGET_ESP32
bool xtouch_xiaomi_ble_test_scan(bool enabled);
bool xtouch_xiaomi_ble_test_sleep(bool enabled);
bool xtouch_xiaomi_ble_test_stop();
bool xtouch_xiaomi_ble_test_start();
int xtouch_xiaomi_ble_test_controller_state();
#endif
