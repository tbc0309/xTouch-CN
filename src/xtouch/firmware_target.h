#pragma once
#include "platform/idf/runtime.h"
#include <sdkconfig.h>
#include "xtouch_board_build.h"
#include "firmware_image.h"
inline bool xtouch_firmware_header_valid(const uint8_t *header,size_t size){return xtouch_application_image(header,size,CONFIG_IDF_FIRMWARE_CHIP_ID);}
#define XTOUCH_FIRMWARE_FILENAME XTOUCH_FIRMWARE_FILENAME_OVERRIDE
#define XTOUCH_FIRMWARE_MAX_BYTES XTOUCH_FIRMWARE_LIMIT_OVERRIDE
