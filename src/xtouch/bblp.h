#ifndef _XLCD_BBLP
#define _XLCD_BBLP
#include "features/config.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *xtouch_bblp_model() {
    return bambuStatus.printer_type[0] ? bambuStatus.printer_type : xTouchConfig.xTouchPrinterModel;
}

bool xtouch_bblp_is_p1p() { return strcmp(xtouch_bblp_model(), "C11") == 0; }

bool xtouch_bblp_is_p1s() { return strcmp(xtouch_bblp_model(), "C12") == 0; }

bool xtouch_bblp_is_p1Series() { return xtouch_bblp_is_p1p() || xtouch_bblp_is_p1s(); }

bool xtouch_bblp_network_chamber() {
    if (xtouch_bblp_is_p1Series())
        return false;
    if (bambuStatus.chamber_support_known)
        return bambuStatus.chamber_supported;
    const char *m = xtouch_bblp_model();
    return !strcmp(m, "BL-P001") || !strcmp(m, "BL-P002") || !strcmp(m, "C13") ||
           !strcmp(m, "N7") || !strcmp(m, "N6") || !strcmp(m, "O1C") || !strcmp(m, "O1C2") ||
           !strcmp(m, "O1D") || !strcmp(m, "O1E") || !strcmp(m, "O1S");
}
bool xtouch_bblp_chamber_display() {
    return xtouch_bblp_network_chamber() ||
           (XTOUCH_FEATURE_EXTERNAL_CHAMBER && xtouch_bblp_is_p1Series() && xTouchConfig.xTouchChamberSensorEnabled);
}
bool ui_printer_model_chamber_heater(const char *value);
bool xtouch_bblp_chamber_heater() {
    return xtouch_bblp_network_chamber() &&
           (bambuStatus.chamber_edit_known ? bambuStatus.chamber_edit_supported
                                           : ui_printer_model_chamber_heater(xtouch_bblp_model()));
}
unsigned xtouch_bblp_chamber_max() {
    unsigned safe = !strcmp(xtouch_bblp_model(), "C13") ? 60 : 65;
    return bambuStatus.chamber_edit_max && bambuStatus.chamber_edit_max < safe
               ? bambuStatus.chamber_edit_max
               : safe;
}
bool xtouch_bblp_fan_supported(unsigned index) {
    if (index > 2) return false;
    if (bambuStatus.fan_new_protocol) return (bambuStatus.fan_protocol_mask & (1U << index)) != 0;
    if (bambuStatus.fan_support_known & (1U << index))
        return bambuStatus.fan_supported & (1U << index);
    const char *m = xtouch_bblp_model();
    if (index && (!strcmp(m, "N1") || !strcmp(m, "N2S")))
        return false;
    if (index == 1 && (!strcmp(m, "C11") || !strcmp(m, "BL-P002")))
        return xTouchConfig.xTouchAuxFanEnabled;
    if (index == 2 && !strcmp(m, "C11"))
        return xTouchConfig.xTouchChamberFanEnabled;
    return true;
}
bool xtouch_bblp_is_x1() {
    return strcmp(xtouch_bblp_model(), "3DPrinter-X1") == 0 ||
           strcmp(xtouch_bblp_model(), "BL-P002") == 0;
}

bool xtouch_bblp_is_x1c() {
    return strcmp(xtouch_bblp_model(), "3DPrinter-X1-Carbon") == 0 ||
           strcmp(xtouch_bblp_model(), "BL-P001") == 0;
}

bool xtouch_bblp_is_x1Series() { return xtouch_bblp_is_x1() || xtouch_bblp_is_x1c(); }

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
