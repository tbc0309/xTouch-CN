#pragma once
struct BoardPowerProfile {
    unsigned minMHz, maxMHz;
    bool lightSleep;
};
#ifdef XTOUCH_BOARD_POWER_HEADER
#include XTOUCH_BOARD_POWER_HEADER
#else
#include "xtouch_board_power.h"
#endif
