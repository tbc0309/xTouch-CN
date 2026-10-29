#pragma once
#include <stdint.h>
// A completed station connection must never leave its boot timeout armed.
inline bool xtouch_wifi_fallback_due(uint32_t now,uint32_t started,bool connected){
 return started&&!connected&&uint32_t(now-started)>=30000u;
}
