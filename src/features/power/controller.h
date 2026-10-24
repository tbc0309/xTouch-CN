#pragma once
/* Queued by HTTP handlers, executed after returning to the UI/main loop. */
bool xtouch_display_request(unsigned mode);
void xtouch_display_poll(bool safeIdle);
