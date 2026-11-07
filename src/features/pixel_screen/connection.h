#pragma once
#include "core.h"
#include "display_policy.h"
#include <ArduinoJson.h>
namespace pixel_screen {
struct ConnectionConfig {
    bool enabled = false;
    char mac[18] = {};
    char password[7] = {};
    unsigned interval = 2;
    unsigned brightness = 80;
    IdleDisplay idleDisplay = IdleDisplay::Clock;
    bool screenOn = true, rotate180 = false;
    bool resetOnConnect = false, clearOnConnect = false;
    bool passwordUnavailable = false;
};
void connectionConfigure(const ConnectionConfig &config);
void connectionSnapshot(const Snapshot &snapshot);
void connectionBegin();
void connectionTimeChanged();
void connectionStatus(JsonObject out);
bool connectionScanStart();
void connectionScanStatus(JsonObject out);
} // namespace pixel_screen
