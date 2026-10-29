#ifndef XTOUCH_NATIVE_IDF_JSON_H
#define XTOUCH_NATIVE_IDF_JSON_H
#pragma once
#include "text.h"
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>
namespace ArduinoJson {
template<> struct Converter<::String> {
 static void toJson(const ::String &source,JsonVariant dest){dest.set(static_cast<const std::string&>(source));}
 static ::String fromJson(JsonVariantConst source){return String(source.as<std::string>());}
 static bool checkJson(JsonVariantConst source){return source.is<const char*>();}
};
}

#endif
