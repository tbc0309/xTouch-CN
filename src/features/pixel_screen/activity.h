#pragma once
#include <stdint.h>
#include "core.h"

namespace pixel_screen {
enum class Activity : uint8_t {
    None,
    Heating,
    Homing,
    Leveling,
    Cleaning,
    Filament,
    Calibration,
    Checking
};
inline Activity stageActivity(int stage) {
    switch (stage) {
    case 2:
    case 7:
    case 49:
    case 50:
    case 54:
    case 58:
    case 63:
        return Activity::Heating;
    case 13:
    case 68:
    case 70:
        return Activity::Homing;
    case 1:
    case 9:
    case 40:
    case 47:
    case 48:
    case 57:
        return Activity::Leveling;
    case 14:
        return Activity::Cleaning;
    case 4:
    case 22:
    case 24:
    case 77:
        return Activity::Filament;
    case 3:
    case 8:
    case 12:
    case 18:
    case 19:
    case 25:
    case 36:
    case 37:
    case 38:
    case 39:
    case 43:
    case 45:
    case 46:
    case 51:
    case 53:
    case 56:
    case 60:
    case 61:
    case 62:
    case 65:
    case 67:
    case 71:
        return Activity::Calibration;
    case 10:
    case 11:
    case 15:
    case 41:
    case 42:
    case 44:
    case 52:
    case 55:
    case 64:
    case 72:
    case 73:
    case 74:
    case 75:
        return Activity::Checking;
    default:
        return Activity::None;
    }
}
inline Activity activityFor(const Snapshot &s) {
    if (s.state != State::Preparing && s.state != State::Printing)
        return Activity::None;
    Activity stage = stageActivity(s.stage);
    // A reported mechanical stage takes precedence over background AMS telemetry.
    if (stage != Activity::None && stage != Activity::Heating)
        return stage;
    // AMS reports are independent of stg_cur: material changes can run while heating.
    if (s.amsMain == 1)
        return Activity::Filament;
    if (s.amsMain == 4)
        return Activity::Calibration;
    if (s.amsMain == 16)
        return Activity::Checking;
    return stage;
}
} // namespace pixel_screen
