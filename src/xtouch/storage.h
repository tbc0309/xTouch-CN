#pragma once
#include "platform/idf/runtime.h"
#include <ArduinoJson.h>

// All persistent UI configuration goes through this API, never directly to SD.
bool xtouch_storage_begin();
bool xtouch_storage_ready();
bool xtouch_storage_has(const char *path);
DynamicJsonDocument xtouch_storage_read(const char *path, size_t capacity = 4096);
bool xtouch_storage_write(const char *path, const JsonDocument &document, bool syncSD = true);
bool xtouch_storage_delete(const char *path);
void xtouch_storage_set_sd(bool available);
bool xtouch_storage_sd_available();
bool xtouch_storage_sync(bool runtime = false);
bool xtouch_storage_restart_required();
void xtouch_storage_clock_ready();
