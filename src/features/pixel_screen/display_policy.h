#pragma once
#include "core.h"

namespace pixel_screen {
class SourceGate {
    uint32_t token = 0, changedAt = 0;
    bool seen = false;

  public:
    bool observe(uint32_t source, bool switching, uint32_t now) {
        if (!seen || token != source || switching) {
            seen = true;
            token = source;
            changedAt = now;
        }
        return !switching && uint32_t(now - changedAt) >= 3000;
    }
};
enum class IdleDisplay : uint8_t { Clock, Alternate };
bool printLayoutValid(const char *name);
bool idleDisplayValid(const char *name);
IdleDisplay idleDisplayFromName(const char *name);
const char *idleDisplayName(IdleDisplay mode);
struct DisplayPlan {
    Snapshot picture;
    bool nativeClock;
};
bool environmentAvailable(const Snapshot &source);
DisplayPlan displayPlan(const Snapshot &source, IdleDisplay mode, uint32_t idleElapsed);
} // namespace pixel_screen
