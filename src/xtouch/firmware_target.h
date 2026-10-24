#pragma once
#include <Arduino.h>
#include <sdkconfig.h>
#include "firmware_image.h"
inline bool xtouch_firmware_header_valid(const uint8_t *header,size_t size){return xtouch_application_image(header,size,CONFIG_IDF_FIRMWARE_CHIP_ID);}

#ifdef XTOUCH_WAVESHARE
#ifdef XTOUCH_BOARD_S3
#define XTOUCH_FIRMWARE_MAX_BYTES (6U*1024U*1024U)
#define XTOUCH_FIRMWARE_FILENAME "/esp32-s3-touch-lcd-2.8-v2.bin"
#else
#define XTOUCH_FIRMWARE_MAX_BYTES (12U*1024U*1024U)
#define XTOUCH_FIRMWARE_FILENAME "/esp32-p4-wifi6-touch-lcd-4.3.bin"
#endif
#else
#define XTOUCH_FIRMWARE_MAX_BYTES 1966080U
#define XTOUCH_FIRMWARE_FILENAME "/xtouch-2.8.bin"
#endif
