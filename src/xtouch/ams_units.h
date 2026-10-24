#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int xtouch_ams_unit_index(int protocol_id);
int xtouch_ams_protocol_id(unsigned index);
unsigned xtouch_ams_slot_count(unsigned index);
int xtouch_ams_tray_index(int protocol_tray);
int xtouch_ams_protocol_tray(int index);
uint64_t xtouch_ams_slot_mask(uint32_t protocol_mask);
const char *xtouch_ams_kind_name(unsigned index);
void xtouch_ams_unit_label(unsigned index, char *out, unsigned capacity);
#ifdef __cplusplus
}
#endif
