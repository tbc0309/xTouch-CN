#include "core.h"
#include "activity.h"
#include "palette.h"
#include "clock_digits.h"
#include "print_layout.h"
#include <string.h>
#include <cmath>
#include <cstdio>
#include <initializer_list>
namespace pixel_screen {
unsigned animationFrames(const Snapshot &s) {
    if (s.state == State::Environment || s.state == State::Cancelled ||
        s.state == State::Paused || s.state == State::Offline)
        return 1;
    return print_layout::active(s) ? print_layout::frames(s) : 24;
}

namespace {
enum : uint8_t {
    Off,
    White = 2,
    Dim,
    Green,
    Blue,
    Yellow,
    Red,
    Cyan,
    Azure,
    Purple,
    Pink,
    Orange,
    Lemon,
    Mint,
    Track,
    Top,
    Side,
    Base,
    Inside,
    Rail,
    BedLight,
    Shadow,
    FeedColor,
    LedGreen,
    LedBlue,
    LedLight,
    Heat,
    Warm,
    DigitLeft,
    DigitRight,
    FilamentColor
};
const uint8_t rainbow[] = {Cyan, Azure, Purple, Pink, Orange, Lemon, Mint};
void dot(uint8_t *f, int x, int y, uint8_t color) {
    if (x >= 0 && x < 32 && y >= 0 && y < 32)
        f[y * 32 + x] = color;
}
void box(uint8_t *f, int x, int y, int w, int h, uint8_t color) {
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            dot(f, x + i, y + j, color);
}
void line(uint8_t *f, int x, int y, int xx, int yy, uint8_t color) {
    int dx = abs(xx - x), sx = x < xx ? 1 : -1, dy = -abs(yy - y), sy = y < yy ? 1 : -1,
        e = dx + dy;
    for (;;) {
        dot(f, x, y, color);
        if (x == xx && y == yy)
            break;
        int k = 2 * e;
        if (k >= dy) {
            e += dy;
            x += sx;
        }
        if (k <= dx) {
            e += dx;
            y += sy;
        }
    }
}
struct Glyph {
    char ch;
    uint16_t bits;
};
const Glyph glyphs[] = {
    {'0', 0x7b6f}, {'1', 0x2c97}, {'2', 0x73e7}, {'3', 0x73cf}, {'4', 0x5bc9}, {'5', 0x79cf},
    {'6', 0x79ef}, {'7', 0x7292}, {'8', 0x7bef}, {'9', 0x7bcf}, {'A', 0x2bed}, {'B', 0x6bae},
    {'C', 0x7927}, {'D', 0x6b6e}, {'E', 0x79a7}, {'F', 0x79a4}, {'H', 0x5bed}, {'I', 0x7497},
    {'L', 0x4927}, {'M', 0x5fed}, {'N', 0x5ffd}, {'O', 0x7b6f}, {'P', 0x6ba4}, {'R', 0x6bad},
    {'S', 0x79cf}, {'T', 0x7492}, {'U', 0x5b6f}, {'V', 0x5b6a}, {'X', 0x5aad}, {'Y', 0x5a92},
    {'Z', 0x72a7}, {'%', 0x52a5}, {':', 0x0410}, {'-', 0x01c0}, {'.', 0x0002}, {'~', 0x6c00},
    {' ', 0x0000},
};
void word(uint8_t *f, const char *text, int x, int y, uint8_t color, int scale = 1) {
    for (unsigned n = 0; text[n]; ++n) {
        uint16_t bits = 0;
        for (const auto &g : glyphs)
            if (g.ch == text[n]) {
                bits = g.bits;
                break;
            }
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 3; ++col)
                if (bits & (1u << (14 - row * 3 - col)))
                    box(f, x + int(n) * 4 * scale + col * scale, y + row * scale, scale, scale,
                        color);
    }
}
void center(uint8_t *f, const char *text, int y, uint8_t color, int scale = 1) {
    word(f, text, int(std::floor((32 - (int(strlen(text)) * 4 - 1) * scale) / 2.0)), y, color,
         scale);
}
// One icon canvas and caption baseline, shared by all four result pages and the 64px scaler.
constexpr int stateIconX = 6, stateIconY = 8, stateIconWidth = 20, stateIconHeight = 16;
constexpr int stateCaptionY = 26;
void stateIcon(uint8_t *f, State state) {
    const int x = stateIconX, y = stateIconY;
    switch (state) {
    case State::Paused:
        box(f, x, y, 5, stateIconHeight, Yellow);
        box(f, x + stateIconWidth - 5, y, 5, stateIconHeight, Yellow);
        break;
    case State::Complete:
        line(f, x, y + 8, x + 7, y + 15, Green);
        line(f, x + 7, y + 15, x + 19, y, Green);
        line(f, x, y + 7, x + 7, y + 14, Green);
        line(f, x + 7, y + 14, x + 19, y, Green);
        break;
    case State::Cancelled:
        for (int row = 0; row < stateIconHeight; ++row)
            for (int col = 0; col < stateIconWidth; ++col)
                if (abs(col * 15 - row * 19) <= 19 || abs((19 - col) * 15 - row * 19) <= 19)
                    dot(f, x + col, y + row, Dim);
        break;
    case State::Error:
        line(f, x + 9, y, x, y + 15, Red);
        line(f, x + 10, y, x + 19, y + 15, Red);
        line(f, x, y + 15, x + 19, y + 15, Red);
        box(f, x + 9, y + 4, 2, 6, Red);
        box(f, x + 9, y + 12, 2, 2, Red);
        break;
    default:
        break;
    }
}
void head(uint8_t *f, int x, int y, unsigned tick, const Snapshot &s) {
    box(f, x, y, 5, 2, White);
    const bool coloured =
        s.filamentKnown && !(s.state == State::Preparing && activityFor(s) == Activity::None);
    if (coloured && !(s.headParked && activityFor(s) == Activity::None))
        dot(f, x + 2, y + (tick / 2 % 2), FeedColor);
    box(f, x + 1, y + 2, 3, 1, Orange);
    dot(f, x + 2, y + 3, coloured ? FilamentColor : Yellow);
}
void chamberHeat(uint8_t *f, const Snapshot &s, unsigned tick) {
    if (!s.chamberHeating || s.headParked)
        return;
    for (int side : {4, 26})
        for (int k = 0; k < 8; ++k) {
            if ((k + tick / 2) % 4 == 0)
                continue;
            int bend = (k + tick / 3) % 4 >= 2;
            dot(f, side + bend, 22 - k, (k + tick) % 3 ? Orange : Yellow);
        }
}
void printer(uint8_t *f, const Snapshot &s, unsigned tick, bool drawHead = true) {
    box(f, 1, 7, 30, 2, Top);
    box(f, 1, 9, 2, 15, Azure);
    box(f, 29, 9, 2, 15, Side);
    box(f, 1, 24, 30, 2, Base);
    box(f, 3, 9, 26, 15, Inside);
    box(f, 4, 10, 24, 1, Rail);
    box(f, 5, 23, 22, 1, BedLight);
    int h = int(modelHeight(s)), top = 23 - h;
    for (int y = top; y < 23; ++y) {
        box(f, 10, y, 12, 1, rainbow[(23 - y) % 7]);
        dot(f, 21, y, Shadow);
    }
    if (drawHead) {
        int step = tick % 24, bounce = step < 12 ? step : 23 - step;
        int x = s.state == State::Preparing ? 14 : s.headParked ? 13 : 8 + bounce;
        head(f, x, s.state == State::Preparing ? 11 : top - 4, tick, s);
    }
    // Continuous side strip between the top frame and the bottom frame.
    const int filled = (int(s.percent) * 15 + 50) / 100;
    for (int y = 9; y < 24; ++y)
        dot(f, 2, y, y >= 24 - filled ? (s.filamentKnown ? FilamentColor : Yellow) : LedBlue);
    dot(f, 27, 8, tick % 8 < 4 ? LedLight : Azure);
    chamberHeat(f, s, tick);
}
void activityPicture(uint8_t *f, const Snapshot &s, unsigned tick, Activity activity) {
    if (activity == Activity::Heating && s.stage == 49) {
        printer(f, s, tick);
        return;
    }
    // Action scenes draw their own head; never leave a second one behind above the clear area.
    printer(f, s, tick, false);
    box(f, 6, 11, 20, 12, Off);
    box(f, 5, 23, 22, 1, BedLight);
    unsigned t = tick % 24;
    int moving = 8 + int(std::floor((std::sin(t / 4.0) + 1) * 5 + .5));
    switch (activity) {
    case Activity::Heating:
        box(f, 8, 22, 16, 1, Orange);
        head(f, 14, 11, tick, s);
        for (int k = 0; k < 3; ++k)
            for (int y = 17; y < 21; ++y)
                dot(f, 9 + k * 6 + ((y + t) % 3 == 0), y, (y + t) % 3 == 0 ? Orange : Yellow);
        break;
    case Activity::Homing:
        // Reach the homing arrow tip, including its leftmost endpoint.
        head(f, 6 + int(t <= 12 ? t : 24 - t), 11, tick, s);
        line(f, 8, 21, 14, 21, Cyan);
        line(f, 8, 21, 8, 15, Cyan);
        dot(f, 7, 16, Cyan);
        dot(f, 9, 16, Cyan);
        break;
    case Activity::Leveling: {
        const int points[] = {8, 16, 23};
        int x = points[t / 8 % 3];
        head(f, x - 2, 18 + (t % 8 > 3 ? 1 : 0), tick, s);
        for (int point : points)
            dot(f, point, 23, point == x ? Green : Blue);
        break;
    }
    case Activity::Cleaning:
        box(f, 9, 22, 15, 1, Yellow);
        for (int x = 10; x < 24; x += 2)
            box(f, x, 20, 1, 2, Orange);
        head(f, 8 + int(t <= 12 ? t : 24 - t), 16, tick, s);
        break;
    case Activity::Filament:
        for (int i = 0; i < 20; ++i) {
            double angle = i * 3.141592653589793 / 10;
            dot(f, 12 + int(std::floor(std::cos(angle) * 4 + .5)),
                16 + int(std::floor(std::sin(angle) * 4 + .5)), Cyan);
        }
        dot(f, 12, 16, s.filamentKnown ? FeedColor : Yellow);
        line(f, 16, 16, 21, 16, Yellow);
        line(f, 21, 16, 21, 19, Yellow);
        box(f, 20, 17 + t % 3, 3, 1, Orange);
        head(f, 19, 19, tick, s);
        break;
    case Activity::Calibration:
        for (int x = 9; x < 24; ++x)
            dot(f, x, 22, x % 3 == t % 3 ? Yellow : Cyan);
        head(f, moving, 18, tick, s);
        for (int k = 0; k < 4; ++k)
            dot(f, 9 + k * 4, 13 + (k + t) % 3, Yellow);
        break;
    case Activity::Checking:
        box(f, 9, 13, 12, 1, Cyan);
        box(f, 9, 20, 12, 1, Cyan);
        box(f, 8, 14, 1, 6, Cyan);
        box(f, 21, 14, 1, 6, Cyan);
        line(f, 21, 21, 24, 23, White);
        box(f, 11, 14 + t % 6, 8, 1, Yellow);
        break;
    default:
        break;
    }
}
} // namespace
unsigned modelHeight(const Snapshot &s) {
    if (s.currentLayer <= 0 || s.totalLayers <= 0)
        return 0;
    const int layer = s.currentLayer > s.totalLayers ? s.totalLayers : s.currentLayer;
    // Round layer progress to ten heights; the head can overlap the rail at the ceiling.
    return 1 +
           unsigned((uint64_t(layer) * 9 + unsigned(s.totalLayers) / 2) / unsigned(s.totalLayers));
}
uint32_t visibleFilamentColor(uint32_t color) {
    color &= 0xffffff;
    if (color == 0xffffff)
        return 0xd0d0d0;
    unsigned r = color >> 16, g = color >> 8 & 255, b = color & 255;
    unsigned peak = r > g ? r : g;
    if (b > peak)
        peak = b;
    if (!peak)
        return 0x101010;
    if (peak < 16)
        return (r * 16 / peak << 16) | (g * 16 / peak << 8) | (b * 16 / peak);
    return color;
}
uint32_t rgb(uint8_t index, uint32_t filamentColor, uint32_t feedColor) {
    if (index == FilamentColor)
        return visibleFilamentColor(filamentColor);
    if (index == FeedColor)
        return visibleFilamentColor(feedColor);
    if (index >= paletteCount)
        return 0;
    return uint32_t(palette[index][0]) << 16 | uint32_t(palette[index][1]) << 8 | palette[index][2];
}
uint32_t crc32(const uint8_t *data, size_t size) {
    uint32_t crc = ~uint32_t(0);
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
void renderScene(const Snapshot &input, unsigned tick, uint8_t *f) {
    if (!f)
        return;
    memset(f, Off, pixels);
    Snapshot s = input;
    if (s.percent > 100)
        s.percent = 100;
    uint8_t ink = s.state == State::Error                                    ? Red
                  : s.state == State::Preparing || s.state == State::Paused  ? Yellow
                  : s.state == State::Offline || s.state == State::Cancelled ? Dim
                                                                             : Green;
    word(f, s.model, 1, 1, White);
    box(f, 29, 1, 2, 5, ink);
    char text[40] = {};
    Activity activity = activityFor(s);
    if (activity != Activity::None) {
        activityPicture(f, s, tick, activity);
        snprintf(text, sizeof(text), "%u%%", unsigned(s.percent));
        center(f, text, 27, s.state == State::Preparing ? Yellow : Green);
        return;
    }
    if (s.state == State::Idle) {
        // Native 5x7 digits: no fractional scaling or uneven stroke widths.
        const int positions[] = {3,9,15,17,23};
        for (unsigned i = 0; i < 5 && s.clock[i]; ++i) {
            if (s.clock[i] == ':') {
                if (tick % 10 < 5) {
                    dot(f, positions[i], 14, rainbow[i % 7]);
                    dot(f, positions[i], 17, rainbow[i % 7]);
                }
            } else if (s.clock[i] == '-') {
                box(f, positions[i] + 1, 15, 3, 1, rainbow[i % 7]);
            } else if (s.clock[i] >= '0' && s.clock[i] <= '9') {
                const uint8_t *rows = clock_digits::rows[s.clock[i] - '0'];
                for (int y = 0; y < 7; ++y)
                    for (int x = 0; x < 5; ++x)
                        if (rows[y] & (1u << (4 - x)))
                            dot(f, positions[i] + x, 12 + y, rainbow[i % 7]);
            }
        }
        center(f, "READY", 26, Green);
        for (unsigned i = 0; i < 5; ++i)
            dot(f, 6 + int(i) * 5, 9, rainbow[(i + tick / 3) % 7]);
        return;
    }
    if (s.state == State::Preparing) {
        printer(f, s, tick);
        snprintf(text, sizeof(text), "%u%%", unsigned(s.percent));
        center(f, text, 27, Yellow);
        return;
    }
    if (s.state == State::Complete) {
        for (unsigned i = 0; i < 14; ++i) {
            int x = (i * 11 + 3) % 30 + 1, y = 7 + (i * 5 + tick) % 17;
            dot(f, x, y, rainbow[i % 7]);
            if (i % 3 == 0)
                dot(f, x, y + 1, rainbow[i % 7]);
        }
        stateIcon(f, s.state);
        center(f, "DONE", stateCaptionY, Green);
        return;
    }
    if (s.state == State::Cancelled) {
        stateIcon(f, s.state);
        center(f, "STOP", stateCaptionY, Dim);
        return;
    }
    if (s.state == State::Error) {
        stateIcon(f, s.state);
        if (tick % 10 < 5) {
            box(f, 2, 8, 2, 2, Orange);
            box(f, 28, 8, 2, 2, Orange);
        }
        center(f, "ERROR", stateCaptionY, Red);
        return;
    }
    if (s.state == State::Offline) {
        line(f, 7, 11, 22, 11, Dim);
        box(f, 6, 12, 2, 5, Dim);
        box(f, 23, 12, 2, 5, Dim);
        line(f, 8, 17, 23, 17, Dim);
        line(f, 8, 8, 24, 23, Red);
        center(f, "OFF", 25, Dim);
        return;
    }
    if (s.state == State::Environment) {
        center(f, "ENV", 8, Blue);
        if (s.environmentKnown)
            snprintf(text, sizeof(text), "%.1f~", s.temperature);
        else
            strcpy(text, "--");
        center(f, text, 16, Warm);
        if (s.environmentKnown)
            snprintf(text, sizeof(text), "%.0f%%", s.humidity);
        else
            strcpy(text, "--");
        center(f, text, 24, Cyan);
        box(f, 1, 15, 1, 5, Heat);
        box(f, 0, 20, 3, 2, Heat);
        dot(f, 28, 23, Cyan);
        box(f, 27, 24, 3, 3, Cyan);
        dot(f, 28, 25, White);
        return;
    }
    if (s.state == State::Paused) {
        stateIcon(f, s.state);
        snprintf(text, sizeof(text), "%u%%", unsigned(s.percent));
        center(f, text, stateCaptionY, Yellow);
        return;
    }
    // Base printer artwork is shared with the native 64px renderer.
    // The 32px entry point adds its selected information layout separately.
    printer(f, s, tick);
    snprintf(text, sizeof(text), "%u%%", unsigned(s.percent));
    center(f, text, 27, Green);
}
} // namespace pixel_screen
