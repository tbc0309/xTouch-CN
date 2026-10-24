#include "ams_units.h"
#include "types.h"
#include <stdbool.h>
#include <stdio.h>

int xtouch_ams_unit_index(int id) {
    return id >= 0 && id < 4        ? id
           : id >= 128 && id <= 135 ? id - 124
           : id >= 16 && id <= 31   ? XTOUCH_MIXED_AMS_UNIT
                                    : -1;
}
int xtouch_ams_protocol_id(unsigned index) {
    return index < 4                       ? (int)index
           : index < XTOUCH_MIXED_AMS_UNIT ? (int)index + 124
           : index == XTOUCH_MIXED_AMS_UNIT
               ? (bambuStatus.mixed_ams_protocol_id >= 16 ? bambuStatus.mixed_ams_protocol_id : 16)
               : -1;
}
unsigned xtouch_ams_slot_count(unsigned index) {
    return index < 4 || index == XTOUCH_MIXED_AMS_UNIT ? 4 : index < XTOUCH_MIXED_AMS_UNIT ? 1 : 0;
}
int xtouch_ams_tray_index(int tray) {
    return tray >= 24 && tray <= 27                        ? XTOUCH_MIXED_AMS_UNIT * 4 + tray - 24
           : tray >= 128 && tray <= 135                    ? (tray - 124) * 4
           : tray >= 0 && tray < XTOUCH_STANDARD_AMS_SLOTS ? tray
           : tray == 254 || tray == 255                    ? tray
                                                           : -1;
}
int xtouch_ams_protocol_tray(int tray) {
    return tray >= XTOUCH_MIXED_AMS_UNIT * 4 && tray < XTOUCH_MAX_AMS_SLOTS
               ? 24 + tray - XTOUCH_MIXED_AMS_UNIT * 4
           : tray >= XTOUCH_STANDARD_AMS_SLOTS && tray < XTOUCH_MIXED_AMS_UNIT * 4 && tray % 4 == 0
               ? tray / 4 + 124
           : tray >= 0 && tray < XTOUCH_STANDARD_AMS_SLOTS ? tray
           : tray == 254 || tray == 255                    ? tray
                                                           : -1;
}
uint64_t xtouch_ams_slot_mask(uint32_t mask) {
    uint64_t result = mask & 0xffff;
    for (unsigned n = 0; n < 8; n++)
        if (mask & (1UL << (16 + n)))
            result |= 1ULL << ((n + 4) * 4);
    for (unsigned n = 0; n < 4; n++)
        if (mask & (1UL << (24 + n)))
            result |= 1ULL << (XTOUCH_MIXED_AMS_UNIT * 4 + n);
    return result;
}
const char *xtouch_ams_kind_name(unsigned index) {
    static const char *names[] = {"AMS ?", "AMS", "AMS Lite", "AMS 2 Pro", "AMS HT", "AMS Lite"};
    return index < XTOUCH_MAX_AMS_UNITS && bambuStatus.ams_kind[index] < 6
               ? names[bambuStatus.ams_kind[index]]
               : names[0];
}
void xtouch_ams_unit_label(unsigned index, char *out, unsigned cap) {
    if (index >= XTOUCH_MAX_AMS_UNITS) {
        snprintf(out, cap, "?");
        return;
    }
    unsigned kind = bambuStatus.ams_kind[index];
    if (index == XTOUCH_MIXED_AMS_UNIT)
        snprintf(out, cap, "L-%u",
                 bambuStatus.mixed_ams_protocol_id >= 16 ? bambuStatus.mixed_ams_protocol_id - 15
                                                         : 1);
    else if (index >= XTOUCH_STANDARD_AMS_UNITS)
        snprintf(out, cap, "HT-%u", index - XTOUCH_STANDARD_AMS_UNITS + 1);
    else if (kind == 2)
        snprintf(out, cap, "L-%u", index + 1);
    else
        snprintf(out, cap, "%s-%c",
                 kind == 1   ? "1"
                 : kind == 3 ? "2"
                 : kind == 2 ? "L"
                             : "?",
                 'A' + index);
}
