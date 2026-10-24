#pragma once
void xtouch_deep_sleep_boot();
void xtouch_deep_sleep_poll(bool safeIdle);
bool xtouch_deep_sleep_available();
int xtouch_deep_sleep_touch_pin();

void xtouch_deep_sleep_request();
