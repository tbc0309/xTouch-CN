#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
bool ui_notice_show(const char *text,unsigned timeout);
void ui_notice_hide(void);
bool ui_notice_visible(void);
void ui_notice_unavailable(void);
#ifdef __cplusplus
}
#endif
