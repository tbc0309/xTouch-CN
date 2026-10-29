#pragma once
#include <stdint.h>
namespace PortraitFrame {
enum class ByteOrder { Native, Swap16 };
// Compose landscape tiles in native portrait scan order. Contiguous destination
// writes reduce PSRAM cache traffic; width is a panel fact supplied by the port.
inline void copy(uint16_t *frame, int width, int x, int y, int w, int h, const uint16_t *pixels,
                 uint16_t mask = 0, ByteOrder order = ByteOrder::Native) {
    for (int xx = 0; xx < w; ++xx) {
        uint16_t *dest = frame + (x + xx) * width + (width - y - h);
        for (int yy = h - 1; yy >= 0; --yy) {
            uint16_t value = pixels[yy * w + xx] ^ mask;
            *dest++ = order == ByteOrder::Swap16 ? uint16_t((value >> 8) | (value << 8)) : value;
        }
    }
}
} // namespace PortraitFrame
