#ifndef _XLCD_PATHS
#define _XLCD_PATHS

static const char *const xtouch_paths_root = "/xtouch";
static const char *const xtouch_paths_config = "/xtouch.json";

static const char *const xtouch_paths_settings = "/xtouch/settings.json";
static const char *const xtouch_paths_touch = "/xtouch/touch.json";

static const char *const xtouch_paths_firmware_ota_json = "/xtouch/ota.json";
#include "ota_endpoint.h"
static const char *const xtouch_paths_firmware_ota_file = XTOUCH_OTA_MANIFEST_URL;
static const char *const xtouch_paths_firmware_ota_fw = "/firmware.bin";

#endif
