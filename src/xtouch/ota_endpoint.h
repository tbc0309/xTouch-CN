#pragma once
#if __has_include("xtouch_ota_channels.h")
#include "xtouch_ota_channels.h"
#endif
#ifndef XTOUCH_OTA_GENERATED

// Keep the HTTP request and the advertised manifest URL on the same target.
#if defined(XTOUCH_BOARD_ES3C35P)
#define XTOUCH_OTA_MANIFEST_PATH "/tbc0309/xTouch-CN/main/boards/lcdwiki-es3c35p/ota.json"
#elif defined(XTOUCH_BOARD_S3)
#define XTOUCH_OTA_MANIFEST_PATH "/tbc0309/xTouch-CN/main/boards/esp32-s3-touch-lcd-2.8-v2/ota.json"
#elif defined(XTOUCH_BOARD_P4)
#define XTOUCH_OTA_MANIFEST_PATH "/tbc0309/xTouch-CN/main/boards/esp32-p4-wifi6-touch-lcd-4.3/ota.json"
#else
#define XTOUCH_OTA_MANIFEST_PATH "/tbc0309/xTouch-CN/main/boards/esp32-2432s028r/ota.json"
#endif
#define XTOUCH_OTA_MANIFEST_URL "https://raw.githubusercontent.com" XTOUCH_OTA_MANIFEST_PATH
static const char *const xtouch_ota_manifest_paths[] = {XTOUCH_OTA_MANIFEST_PATH};
#endif
