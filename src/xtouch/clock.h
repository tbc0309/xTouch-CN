#pragma once
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Configured local time, available only after this boot's NTP sync.
bool xtouch_clock_text(char *text, size_t size);
#ifdef __cplusplus
}
#endif
