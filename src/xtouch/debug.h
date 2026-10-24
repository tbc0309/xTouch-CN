#ifndef _XLCD_DEBUG
#define _XLCD_DEBUG

#define XTOUCH_DEBUG_LOG true
#define XTOUCH_DEBUG_INFO true
#define XTOUCH_DEBUG_DEBUG false
#define XTOUCH_DEBUG_ERROR true

#define ConsoleLog                                                                         \
    if (XTOUCH_DEBUG_LOG) \
    Serial
#define ConsoleInfo                                                    \
    if (XTOUCH_DEBUG_INFO) \
    Serial
#define ConsoleDebug                              \
    if (XTOUCH_DEBUG_DEBUG) \
    Serial
#define ConsoleError        \
    if (XTOUCH_DEBUG_ERROR) \
    Serial

#include <Arduino.h>
#include <ArduinoJson.h>

void xtouch_debug_json(const JsonDocument &doc)
{
#if XTOUCH_DEBUG_DEBUG == true
    String output;
    serializeJsonPretty(doc, output);
    ConsoleDebug.println(output);
#endif
}

#endif
