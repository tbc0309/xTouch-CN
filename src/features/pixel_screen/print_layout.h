#pragma once
#include "activity.h"
#include "core.h"
#include "layout_data.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <initializer_list>
namespace pixel_screen {
namespace print_layout {
enum class Role { Plain, Number, Separator, Layer, Nozzle, Bed };
inline bool active(const Snapshot &s) {
    return s.state == State::Preparing || s.state == State::Printing;
}
inline bool warm(const Snapshot &s) { return s.state == State::Preparing; }
inline bool dashboard(const Snapshot &s) {
    return s.state == State::Printing && activityFor(s) == Activity::None && s.progressLayout;
}
inline const Glyph &glyph(char c) {
    for (const auto &g : font)
        if (g.ch == c)
            return g;
    return c == ' ' ? font[0] : glyph(' ');
}
inline int width(const char *s, unsigned scale = 1) {
    int n = 0;
    for (; *s; s++)
        n += (glyph(*s).width + 1) * int(scale);
    return n ? n - int(scale) : 0;
}
inline void pixel(uint8_t *f, int x, int y, uint8_t c) {
    if (x >= 0 && x < 32 && y >= 0 && y < 32)
        f[y * 32 + x] = c;
}
inline void box(uint8_t *f, int x, int y, int w, int h, uint8_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            pixel(f, x + i, y + j, c);
}
struct Values {
    char layer[32], clock[24], duration[24], nozzle[16], bed[16];
    explicit Values(const Snapshot &s) {
        if (s.currentLayer < 0 || s.totalLayers <= 0)
            std::strcpy(layer, "--/--");
        else
            std::snprintf(layer, sizeof(layer), "%d/%d", s.currentLayer, s.totalLayers);
        if (s.leftMinutes < 0) {
            std::strcpy(clock, "--:--");
            std::strcpy(duration, "--");
        } else {
            int hours = s.leftMinutes / 60, minutes = s.leftMinutes % 60;
            std::snprintf(clock, sizeof(clock), "%d:%02d", hours, minutes);
            if (hours)
                std::snprintf(duration, sizeof(duration), "%dH%02dM", hours, minutes);
            else
                std::snprintf(duration, sizeof(duration), "%dM", minutes);
        }
        if (s.nozzle < 0)
            std::strcpy(nozzle, "--");
        else
            std::snprintf(nozzle, sizeof(nozzle), "%d", s.nozzle);
        if (s.bed < 0)
            std::strcpy(bed, "--");
        else
            std::snprintf(bed, sizeof(bed), "%d", s.bed);
    }
};
inline unsigned scrollCycle(int size, int available) {
    return size <= available ? 0u : scrollHold * 2 + unsigned(size - available) * scrollStep * 2;
}
inline int offset(int size, int available, unsigned tick, unsigned period) {
    const int distance = std::max(0, size - available);
    if (!distance)
        return 0;
    unsigned cycle = scrollHold * 2 + unsigned(distance) * scrollStep * 2,
             phase = tick % std::max(period, cycle);
    if (phase >= cycle || phase < scrollHold)
        return 0;
    phase -= scrollHold;
    if (phase < unsigned(distance) * scrollStep)
        return int(phase / scrollStep);
    phase -= unsigned(distance) * scrollStep;
    if (phase < scrollHold)
        return distance;
    return distance - std::min(distance, int((phase - scrollHold) / scrollStep));
}
inline uint8_t ink(Role role, const Snapshot &s, int y) {
    bool top = y < 3;
    if (role == Role::Layer || role == Role::Nozzle || role == Role::Bed) {
        if (warm(s))
            return top ? PrepLabelTop : PrepLabelBottom;
        return role == Role::Layer    ? (top ? LayerTop : LayerBottom)
               : role == Role::Nozzle ? (top ? NozzleTop : NozzleBottom)
                                      : (top ? BedTop : BedBottom);
    }
    if (role == Role::Number)
        return warm(s) ? (top ? PrepNumberTop : PrepNumberBottom) : (top ? 2 : NumberBottom);
    if (role == Role::Separator)
        return warm(s) ? (top ? PrepSeparatorTop : PrepSeparatorBottom)
                       : (top ? SeparatorTop : SeparatorBottom);
    return warm(s) ? PrepareYellow : 2;
}
inline int text(uint8_t *f, const char *s, int x, int y, Role role, const Snapshot &snapshot,
                int left = 0, int right = 32) {
    for (; *s; s++) {
        const auto &g = glyph(*s);
        for (int dy = 0; dy < 5; dy++)
            for (int dx = 0; dx < g.width; dx++)
                if (g.rows[dy] & (1u << (g.width - 1 - dx))) {
                    int px = x + dx;
                    if (px >= left && px < right)
                        pixel(f, px, y + dy,
                              ink(*s == '/' || *s == ':' ? Role::Separator : role, snapshot, dy));
                }
        x += g.width + 1;
    }
    return x;
}
inline void plain(uint8_t *f, const char *s, int x, int y, uint8_t color, int scale = 1) {
    for (; *s; s++) {
        const auto &g = glyph(*s);
        for (int dy = 0; dy < 5; dy++)
            for (int dx = 0; dx < g.width; dx++)
                if (g.rows[dy] & (1u << (g.width - 1 - dx)))
                    box(f, x + dx * scale, y + dy * scale, scale, scale, color);
        x += (g.width + 1) * scale;
    }
}
inline void field(uint8_t *f, const char *label, const char *value, Role role, int x, int y,
                  int available, const Snapshot &s, unsigned tick, unsigned period) {
    text(f, label, x, y, role, s, x, x + available);
    int begin = x + 4;
    text(f, value, begin - offset(width(value), available - 4, tick, period), y, Role::Number, s,
         begin, x + available);
}
inline const char *title(const Snapshot &s) {
    switch (activityFor(s)) {
    case Activity::Heating:
        return s.stage == 49 ? "CH HEAT" : "HEAT";
    case Activity::Homing:
        return "HOME";
    case Activity::Leveling:
        return "LEVEL";
    case Activity::Cleaning:
        return "CLEAN";
    case Activity::Filament:
        return "FEED";
    case Activity::Calibration:
        return "CALIB";
    case Activity::Checking:
        return "CHECK";
    default:
        return warm(s) || s.headParked ? "WAIT" : s.model;
    }
}
enum class Pane { Title, Layer, Temperatures };
struct Plan {
    Pane panes[3];
    unsigned duration[3], count, total;
};
inline Plan plan(const Snapshot &s, const Values &v) {
    unsigned layer = std::max(headerHold, scrollCycle(width(v.layer), 23));
    unsigned temps = std::max(headerHold, scrollCycle(9 + width(v.nozzle) + width(v.bed), 27));
    if (dashboard(s))
        return {{Pane::Title, Pane::Temperatures, Pane::Title},
                {headerHold, temps, 0},
                2,
                headerHold + temps};
    return {{Pane::Title, Pane::Layer, Pane::Temperatures},
            {headerHold, layer, temps},
            3,
            headerHold + layer + temps};
}
inline unsigned frames(const Snapshot &s) {
    Values v(s);
    auto p = plan(s, v);
    return std::max(
        minimumCycleFrames,
        std::max(p.total, std::max(scrollCycle(width(v.layer), 26),
                                   scrollCycle(width(dashboard(s) ? v.duration : v.clock),
                                               dashboard(s) ? 30 : 15))));
}
inline void header(uint8_t *f, const Snapshot &s, const Values &v, unsigned tick, unsigned period) {
    box(f, 0, 0, 32, 7, 0);
    auto p = plan(s, v);
    unsigned phase = tick % p.total, index = 0;
    while (index + 1 < p.count && phase >= p.duration[index])
        phase -= p.duration[index++];
    switch (p.panes[index]) {
    case Pane::Layer:
        field(f, "L", v.layer, Role::Layer, 1, 1, 27, s, phase, period);
        break;
    case Pane::Temperatures: {
        int size = 9 + width(v.nozzle) + width(v.bed), x = 1 - offset(size, 27, phase, period);
        x = text(f, "N", x, 1, Role::Nozzle, s, 1, 28);
        x = text(f, v.nozzle, x, 1, Role::Number, s, 1, 28);
        x = text(f, "B", x, 1, Role::Bed, s, 1, 28);
        text(f, v.bed, x, 1, Role::Number, s, 1, 28);
        break;
    }
    default: {
        const char *label = title(s);
        plain(f, label, 1, 1, warm(s) ? PrepareYellow : 2);
        if (label == s.model) {
            char current[16];
            if (s.currentLayer < 0)
                std::strcpy(current, "--");
            else
                std::snprintf(current, sizeof(current), "%d", s.currentLayer);
            const char *value = dashboard(s) ? v.nozzle : current;
            int size = 4 + width(value);
            if (width(label) + 1 + size <= 27)
                field(f, dashboard(s) ? "N" : "L", value, dashboard(s) ? Role::Nozzle : Role::Layer,
                      28 - size, 1, size, s, phase, period);
        }
    }
    }
    box(f, 29, 1, 2, 5, warm(s) ? PrepareYellow : PrintGreen);
}
inline void draw(const Snapshot &s, unsigned tick, uint8_t *f) {
    if (!active(s))
        return;
    Values v(s);
    unsigned period = frames(s);
    if (dashboard(s))
        std::memset(f, 0, pixels);
    header(f, s, v, tick, period);
    if (!dashboard(s)) {
        box(f, 0, 25, 32, 7, 0);
        char percent[8];
        std::snprintf(percent, sizeof(percent), "%u%%", unsigned(s.percent));
        plain(f, percent, 1, 26, warm(s) ? PrepareYellow : PrintGreen);
        text(f, v.clock, std::max(16, 31 - width(v.clock)) -
                             offset(width(v.clock), 15, tick, period),
             26, Role::Number, s, 16, 31);
        return;
    }
    char percent[8];
    std::snprintf(percent, sizeof(percent), "%u%%", unsigned(s.percent));
    plain(f, percent, (32 - width(percent, 2)) / 2, 7, PrintGreen, 2);
    box(f, 1, 18, 30, 1, s.filamentKnown ? 31 : 6);
    unsigned fill = (unsigned(s.percent) * 30 + 50) / 100;
    // Complete one slow rainbow circuit per animation loop; unfilled pixels use the current
    // filament colour.
    constexpr unsigned bands = sizeof(progressRainbow) / sizeof(progressRainbow[0]);
    unsigned shift = (tick % period) * (bands * rainbowBandPixels) / period;
    for (unsigned x = 0; x < fill; x++)
        pixel(f, 1 + int(x), 18, progressRainbow[((x + bands * rainbowBandPixels - shift) % (bands * rainbowBandPixels)) / rainbowBandPixels]);
    if (s.chamberHeating && !s.headParked)
        for (int x : {0, 31})
            for (int y = 8; y < 16; y++)
                if ((y + tick / 2) % 4)
                    pixel(f, x, y, (y + tick) % 3 ? 12 : 6);
    int size = 4 + width(v.layer), start = size <= 30 ? std::max(1, (32 - size) / 2) : 1;
    field(f, "L", v.layer, Role::Layer, start, 20, size > 30 ? 30 : size, s, tick, period);
    text(f, v.duration,
         std::max(1, (32 - width(v.duration)) / 2) - offset(width(v.duration), 30, tick, period),
         26, Role::Number, s, 1, 31);
}
} // namespace print_layout
} // namespace pixel_screen
