#pragma once
#include "core.h"
namespace pixel_screen {
// Last-layer retraction parks the ordinary head, not the reported mechanical action.
// New material or a new task resumes normal motion; FINISH remains authoritative.
class HeadMotion {
    bool ending = false, resuming = false;
    int previousLayer = -1, previousCurrent = -1, previousTarget = -1;
    unsigned previousPercent = 0;
public:
    void reset() {
        ending = resuming = false;
        previousLayer = previousCurrent = previousTarget = -1;
        previousPercent = 0;
    }
    bool observe(const Snapshot &s, int currentTray, int targetTray) {
        if (s.state == State::Preparing || s.state == State::Offline || s.state == State::Idle ||
            s.state == State::Complete || s.state == State::Cancelled ||
            s.currentLayer < previousLayer || s.percent < previousPercent)
            ending = resuming = false;
        const bool newTarget = targetTray >= 0 && targetTray != 255 && previousTarget >= 0 &&
                               targetTray != previousTarget;
        const bool reloaded = currentTray >= 0 && currentTray != 255 && previousCurrent == 255;
        previousLayer = s.currentLayer;
        previousPercent = s.percent;
        previousCurrent = currentTray;
        previousTarget = targetTray;
        if (s.state != State::Printing)
            return false;
        const bool finalLayer = (s.totalLayers > 0 && s.currentLayer >= s.totalLayers) ||
                                s.percent == 100;
        const bool retracting = s.stage == 22 || (s.amsMain == 1 && targetTray == 255);
        const bool empty = !s.filamentKnown && currentTray == 255 && targetTray == 255;
        if (newTarget || reloaded) {
            ending = false;
            resuming = true;
        }
        // An old unloading stage may arrive after the new load target; do not re-latch it.
        if (!retracting && !empty)
            resuming = false;
        if (finalLayer && !resuming && (retracting || empty))
            ending = true;
        return ending;
    }
};
}
