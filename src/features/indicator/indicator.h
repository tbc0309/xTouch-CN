#pragma once
#include <stdint.h>
void xtouch_indicator_begin();
void xtouch_indicator_poll(bool connected,bool warning,bool sleeping,bool enabled);
bool xtouch_indicator_available();
uint32_t xtouch_indicator_color();
