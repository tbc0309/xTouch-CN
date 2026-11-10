#pragma once
#include "activity.h"
#include "xtouch/types.h"

namespace pixel_screen {
struct FilamentColors {
    uint32_t current = 0, feed = 0;
    bool known = false;
};
inline bool trayColor(const XTouchBambuStatus &s, int tray, uint32_t &color) {
    if (tray >= 0 && tray < XTOUCH_MAX_AMS_SLOTS && s.ams &&
        (s.tray_exist_bits & (uint64_t(1) << tray)) && s.ams_tray_color_valid[tray]) {
        color = s.ams_tray_color[tray];
        return true;
    }
    if (tray == 254 && s.external_present && (s.external_color_valid & 1) &&
        (!s.external_multi_protocol || (s.external_materials & 1))) {
        color = s.external_color;
        return true;
    }
    if (tray == 253 && s.external_multi_protocol && (s.external_materials & 2) &&
        (s.external_color_valid & 2)) {
        color = s.external_color2;
        return true;
    }
    return false;
}
inline FilamentColors filamentColors(const XTouchBambuStatus &s) {
    FilamentColors colors;
    int current = s.m_tray_now;
    if (s.nozzle_count > 1) {
        current = s.active_nozzle < 2 && (s.nozzle_loaded_valid & (1U << s.active_nozzle))
                      ? s.nozzle_loaded[s.active_nozzle]
                      : -1;
    } else if (s.nozzle_count == 1 && (s.nozzle_loaded_valid & 1))
        current = s.nozzle_loaded[0];
    colors.known = trayColor(s, current, colors.current);
    colors.feed = colors.current;
    if (stageActivity(s.print_stage_current) != Activity::Filament)
        return colors;
    int target = s.m_tray_tar;
    if (s.nozzle_count > 1) {
        int extruder = target >= 0 && target < XTOUCH_MAX_AMS_SLOTS
                           ? s.ams_bound_extruder[target / 4]
                       : target == 254 ? 0
                       : target == 253 ? 1
                                       : -1;
        if (s.active_nozzle >= 2 || extruder != s.active_nozzle)
            target = -1;
    }
    if (trayColor(s, target, colors.feed) && !colors.known) {
        colors.current = colors.feed;
        colors.known = true;
    }
    return colors;
}
} // namespace pixel_screen
