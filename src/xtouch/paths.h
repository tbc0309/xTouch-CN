#ifndef _XLCD_PATHS
#define _XLCD_PATHS

const char *xtouch_paths_root = "/xtouch";
const char *xtouch_paths_config = "/xtouch.json";

const char *xtouch_paths_settings = "/xtouch/settings.json";
const char *xtouch_paths_touch = "/xtouch/touch.json";

const char *xtouch_paths_firmware_ota_json = "/xtouch/ota.json";
#include "ota_endpoint.h"
const char *xtouch_paths_firmware_ota_file = XTOUCH_OTA_MANIFEST_URL;
const char *xtouch_paths_firmware_ota_fw = "/firmware.bin";

#endif
