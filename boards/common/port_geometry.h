#pragma once
#include "port_profile.h"
#include <algorithm>
namespace XTouchPort {
inline bool touchPoint(const Profile &p, Touch detected, int &x, int &y) {
    bool mirrorX = p.touchMirrorX;
    // CHD variants use opposite X polarity for the FT and GT controller options.
    // Keep any panel-specific coordinate offset out until calibrated on the actual module.
    if (p.touch == Touch::FtOrGt && detected == Touch::Gt911)
        mirrorX = !mirrorX;
    if (p.touchSwap)
        std::swap(x, y);
    if (mirrorX)
        x = p.width - 1 - x;
    if (p.touchMirrorY)
        y = p.height - 1 - y;
    return x >= 0 && y >= 0 && x < p.width && y < p.height;
}
} // namespace XTouchPort
