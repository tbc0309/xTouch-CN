#pragma once
#include "features/config.h"
#if XTOUCH_FEATURE_EXTERNAL_CHAMBER
void xtouch_chamber_timer_init();
void xtouch_chamber_timer_create();
void xtouch_chamber_timer_start();
void xtouch_chamber_timer_stop();
#else
inline void xtouch_chamber_timer_init() {}
inline void xtouch_chamber_timer_create() {}
inline void xtouch_chamber_timer_start() {}
inline void xtouch_chamber_timer_stop() {}
#endif
