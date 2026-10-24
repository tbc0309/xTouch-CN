#pragma once
#include <ArduinoJson.h>

// Optional S3/P4 BLE provisioning service. No credential or network identifiers in diagnostics.
void board_ble_begin();
void board_ble_status(JsonObject out);
bool board_ble_request(bool start);
bool board_ble_busy();
// Called from the portal loop, never from the SDK event task.
bool board_ble_finished(char *ssid, size_t ssidSize, char *password, size_t passwordSize);
