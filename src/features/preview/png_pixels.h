#pragma once
#include <PNGdec.h>
#include <cstddef>
#include <cstdint>

namespace XTouchPngPixels {
// LVGL RGB565A8: a straight RGB565 plane followed by one 8-bit alpha plane.
// Keep original colors at translucent edges; compositing belongs to LVGL.
inline size_t bytes(unsigned w, unsigned h) {
    return size_t(w) * h * 3;
}
inline uint8_t *alpha(uint16_t *pixels, unsigned w, unsigned h) {
    return reinterpret_cast<uint8_t *>(pixels) + size_t(w) * h * 2;
}
inline bool row(PNG &png, const PNGDRAW &line, uint16_t *color, uint8_t *opacity, unsigned width,
                unsigned sourceWidth) {
    if (!width || !sourceWidth || unsigned(line.iWidth) != sourceWidth || !line.pPixels)
        return false;
    const unsigned bits = unsigned(line.iBpp);
    if (bits != 1 && bits != 2 && bits != 4 && bits != 8)
        return false;
    const uint32_t key = png.getTransparentColor();
    for (unsigned x = 0; x < width; ++x) {
        unsigned sx = unsigned(uint64_t(x) * sourceWidth / width);
        const uint8_t *p = line.pPixels;
        uint8_t r = 0, g = 0, b = 0, a = 255;
        switch (line.iPixelType) {
        case PNG_PIXEL_TRUECOLOR_ALPHA:
            if (bits != 8)
                return false;
            p += sx * 4;
            r = p[0];
            g = p[1];
            b = p[2];
            a = p[3];
            break;
        case PNG_PIXEL_TRUECOLOR:
            if (bits != 8)
                return false;
            p += sx * 3;
            r = p[0];
            g = p[1];
            b = p[2];
            if (line.iHasAlpha && ((uint32_t(r) << 16) | (uint32_t(g) << 8) | b) == key)
                a = 0;
            break;
        case PNG_PIXEL_GRAY_ALPHA:
            if (bits != 8)
                return false;
            p += sx * 2;
            r = g = b = p[0];
            a = p[1];
            break;
        case PNG_PIXEL_GRAYSCALE:
        case PNG_PIXEL_INDEXED: {
            unsigned mask = (1u << bits) - 1;
            unsigned sample = (p[sx * bits / 8] >> (8 - bits - sx * bits % 8)) & mask;
            if (line.iPixelType == PNG_PIXEL_INDEXED) {
                if (!line.pPalette)
                    return false;
                const uint8_t *entry = line.pPalette + sample * 3;
                r = entry[0];
                g = entry[1];
                b = entry[2];
                if (line.iHasAlpha)
                    a = line.pPalette[768 + sample];
            } else {
                r = g = b = uint8_t(sample * 255 / mask);
                if (line.iHasAlpha && sample == key)
                    a = 0;
            }
            break;
        }
        default:
            return false;
        }
        color[x] = uint16_t((uint16_t(r >> 3) << 11) | (uint16_t(g >> 2) << 5) | (b >> 3));
        opacity[x] = a;
    }
    return true;
}
} // namespace XTouchPngPixels
