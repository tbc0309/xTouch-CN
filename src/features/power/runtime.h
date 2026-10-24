#pragma once
#include <stdint.h>
struct XTouchPowerStatus { bool screenSleeping, automaticDfs, automaticLightSleep; unsigned minMHz,maxMHz; int error; uint32_t wakeCause; };
void xtouch_power_begin();
bool xtouch_power_woke_from_touch();
void xtouch_power_screen(bool sleeping);
XTouchPowerStatus xtouch_power_status();
void xtouch_power_ui_begin();
void xtouch_power_ui_end();
