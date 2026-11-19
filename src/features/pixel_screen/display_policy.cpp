#include "display_policy.h"
#include <cmath>
#include <cstring>

namespace pixel_screen {
bool printLayoutValid(const char *name) {
    return name && (!strcmp(name, "printer") || !strcmp(name, "progress"));
}
bool idleDisplayValid(const char *name) {
    return name && (!strcmp(name, "clock") || !strcmp(name, "alternate"));
}
IdleDisplay idleDisplayFromName(const char *name) {
    return name && !strcmp(name, "alternate") ? IdleDisplay::Alternate : IdleDisplay::Clock;
}
const char *idleDisplayName(IdleDisplay mode) {
    return mode == IdleDisplay::Alternate ? "alternate" : "clock";
}
bool environmentAvailable(const Snapshot &s) {
    return s.environmentKnown && std::isfinite(s.temperature) && s.temperature >= -40 &&
           s.temperature <= 85 && std::isfinite(s.humidity) && s.humidity >= 0 && s.humidity <= 100;
}
DisplayPlan displayPlan(const Snapshot &source, IdleDisplay mode, uint32_t idleElapsed) {
    DisplayPlan plan{source, false};
    if ((source.state == State::Complete || source.state == State::Cancelled) &&
        idleElapsed >= 30000) {
        plan.picture.state = State::Idle;
        idleElapsed -= 30000;
    }
    if (plan.picture.state != State::Idle)
        return plan;
    if (environmentAvailable(source) && mode == IdleDisplay::Alternate && idleElapsed / 10000 % 2) {
        plan.picture.state = State::Environment;
        return plan;
    }
    // Until network time is available, retain the unknown-time artwork.
    const char *t = source.clock;
    plan.nativeClock = t[0] >= '0' && t[0] <= '2' && t[1] >= '0' && t[1] <= '9' && t[2] == ':' &&
                       t[3] >= '0' && t[3] <= '5' && t[4] >= '0' && t[4] <= '9' &&
                       (t[0] != '2' || t[1] <= '3');
    return plan;
}
} // namespace pixel_screen
