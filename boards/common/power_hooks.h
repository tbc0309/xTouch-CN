#pragma once
struct BoardPowerProfile {unsigned minMHz,maxMHz;bool lightSleep;};
#ifdef XTOUCH_BOARD_S3
#include "../esp32-s3-touch-lcd-2.8-v2/power.h"
#elif defined(XTOUCH_BOARD_P4)
#include "../esp32-p4-wifi6-touch-lcd-4.3/power.h"
#else
#include "../esp32-2432s028r/power.h"
#endif
