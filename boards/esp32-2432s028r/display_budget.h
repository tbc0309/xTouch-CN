#pragma once
#include <stddef.h>
#include <stdint.h>

namespace XTouchCydDisplay {
constexpr unsigned Width = 320;
constexpr unsigned Height = 240;
constexpr unsigned DrawRows = 16;
constexpr unsigned DrawPixels = Width * DrawRows;
constexpr size_t DrawBytes = DrawPixels * sizeof(uint16_t);
static_assert(Height % DrawRows == 0, "Whole-screen refresh must use complete tiles");
static_assert(DrawBytes == 10240, "Review internal RAM before changing the CYD buffer budget");
inline bool validRegion(int x, int y, int width, int height) {
    // Subtraction avoids overflow for malformed coordinates or dimensions.
    return x >= 0 && y >= 0 && width > 0 && height > 0 && x < int(Width) && y < int(Height) &&
           width <= int(Width) - x && height <= int(Height) - y;
}
inline bool fitsDma(int x, int y, int width, int height) {
    return validRegion(x, y, width, height) && unsigned(width) * unsigned(height) <= DrawPixels;
}
} // namespace XTouchCydDisplay
