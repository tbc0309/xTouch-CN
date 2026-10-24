#pragma once
#include <ArduinoJson.h>
constexpr const char *xtouch_environment_config_path="/xtouch/env.json";
static_assert(sizeof("/xtouch/env.json.previous")<=32,"SPIFFS backup path must fit");
void xtouch_environment_begin();
void xtouch_environment_status(JsonObject out);
bool xtouch_environment_valid(JsonVariantConst doc);
bool xtouch_environment_store(JsonVariantConst doc);
bool xtouch_environment_save(JsonVariantConst input);
void xtouch_xiaomi_ble_unbind();
bool xtouch_xiaomi_ble_measurement(float &temperature,float &humidity,uint32_t &age,uint32_t *temperatureAge=nullptr,uint32_t *humidityAge=nullptr,uint32_t *seenAge=nullptr);

const char *xtouch_xiaomi_ble_model();
