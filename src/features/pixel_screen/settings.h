#pragma once
#include <ArduinoJson.h>
#include "core.h"
namespace pixel_screen {
constexpr const char *config_path = "/xtouch/pixel.json";
void begin();
bool valid(JsonVariantConst input, bool legacy = false);
bool save(JsonVariantConst input);
bool store(JsonVariantConst input);
void status(JsonObject out);
void source(unsigned slot, const char *identity, bool switching, uint32_t now);
void update(bool online, int printState, int percent, bool fault, bool preparing = false,
            int currentLayer = -1, int totalLayers = 0, int stage = -1, uint32_t filamentColor = 0,
            uint32_t feedColor = 0, bool filamentKnown = false, int amsMain = -1, int currentTray = -1, int targetTray = -1, bool chamberHeating = false);
void telemetry(const char *model, const char *clock, int leftMinutes, int nozzle, int bed,
               bool environmentKnown, double temperature, double humidity);
Snapshot snapshot();
const char *stateName(State value);
} // namespace pixel_screen
