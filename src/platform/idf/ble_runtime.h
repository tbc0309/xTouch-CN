#pragma once
#include "features/config.h"
#if XTOUCH_FEATURE_XIAOMI_BLE || XTOUCH_FEATURE_PIXEL_SCREEN
#include <host/ble_gap.h>
#include <ArduinoJson.h>
namespace xtouch_ble {
using Observer = int (*)(ble_gap_event *, void *);
void observer(unsigned slot, Observer callback);
void begin();
void demand(unsigned slot, bool enabled);
// Acquire only a ready host; release after every connection close, including failure.
bool clientActive(bool active);
bool initialized();
void status(JsonObject out);
void poll();
bool requestActiveScan();
uint32_t scanRemainingMs();
bool ready();
bool scanning();
int error();
uint8_t addressType();
bool pauseScan(bool pause);
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION && CONFIG_IDF_TARGET_ESP32
bool testStop();
#endif
} // namespace xtouch_ble
#endif
