#pragma once
#include "activity.h"
#include "core.h"
#include <cstdio>
namespace pixel_screen {
struct PictureKeys {
    uint32_t scene, progress, full;
};
// Only data painted by the current scene participates in upload scheduling.
// Animation advances on the panel; elapsed time is deliberately not a change.
inline PictureKeys pictureKeys(const Snapshot &s) {
    uint32_t hash = 2166136261u;
    auto add = [&](uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) {
            hash = (hash ^ uint8_t(value)) * 16777619u;
            value >>= 8;
        }
    };
    auto text = [&](const char *value, size_t capacity) {
        for (size_t i = 0; i < capacity && value[i]; ++i) add(uint8_t(value[i]));
    };
    add(unsigned(s.state));
    text(s.model, sizeof(s.model));
    const bool active = s.state == State::Preparing || s.state == State::Printing;
    if (active) {
        const Activity activity = activityFor(s);
        add(unsigned(activity));
        add(activity == Activity::Heating && s.stage == 49);
        add(s.headParked);
        add(s.chamberHeating);
        add(s.progressLayout);
        add(s.filamentKnown ? visibleFilamentColor(s.filamentColor) : 0);
        const bool head = activity != Activity::Checking && s.filamentKnown &&
                          !(s.state == State::Preparing && activity == Activity::None);
        add(head ? visibleFilamentColor(s.feedColor) : 0);
    }
    PictureKeys keys{hash, 0, 0};
    if (active || s.state == State::Paused) add(s.percent > 100 ? 100 : s.percent);
    if (active) add(modelHeight(s));
    keys.progress = hash;
    if (active) {
        add(unsigned(s.currentLayer));
        add(unsigned(s.totalLayers));
        add(unsigned(s.leftMinutes));
        add(unsigned(s.nozzle));
        add(unsigned(s.bed));
    } else if (s.state == State::Idle) {
        text(s.clock, sizeof(s.clock));
    } else if (s.state == State::Environment) {
        add(s.environmentKnown);
        if (s.environmentKnown) {
            char values[40];
            std::snprintf(values, sizeof(values), "%.1f/%.0f", s.temperature, s.humidity);
            text(values, sizeof(values));
        }
    }
    keys.full = hash;
    return keys;
}
} // namespace pixel_screen
