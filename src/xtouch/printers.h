#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define XTOUCH_MAX_PRINTERS 100

#ifdef __cplusplus
extern "C" {
#endif
bool xtouch_printer_configured(uint8_t slot);
uint8_t xtouch_printer_active(void);
const char *xtouch_printer_name(void);
const char *xtouch_printer_name_at(uint8_t slot);
const char *xtouch_printer_model_at(uint8_t slot);
bool xtouch_printer_request(uint8_t slot);
bool xtouch_printer_switch_pending(void);
bool xtouch_printer_online(void);
bool xtouch_printer_cloud(void);
bool xtouch_printer_camera_credentials(char *host,size_t hostSize,char *code,size_t codeSize);
bool xtouch_printer_local_credentials(char *host,size_t hostSize,char *code,size_t codeSize);
// Explicit validated edit target for the next config reload; never a serial-only fallback.
void xtouch_printer_edit_hint(const char *serial,const char *host);
// 0: LAN, 1: imported token, 2: account sign-in.
uint8_t xtouch_printer_source(uint8_t slot);
int xtouch_printer_cloud_health(uint8_t slot);
const char *xtouch_printer_account(void);
void xtouch_printer_set_cloud_health(const char *account,int state,const char *serials);
uint32_t xtouch_printer_source_color(uint8_t slot);
#ifdef __cplusplus
}
#endif
