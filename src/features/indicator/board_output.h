#pragma once
#include <stdint.h>
// A board owns its electrical implementation; the feature owns state colours.
extern "C" bool xtouch_board_indicator_begin();
extern "C" bool xtouch_board_indicator_rgb(uint8_t red, uint8_t green, uint8_t blue);
