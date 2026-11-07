// Compiled only for a 32x32 display: layout entry point and BLE PNG/GIF transport.
#include "core.h"
#include "palette.h"
#include "print_layout.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
namespace pixel_screen {
void renderScene(const Snapshot &, unsigned, uint8_t *);
namespace {
void be32(uint8_t *p, uint32_t n) {
    p[0] = uint8_t(n >> 24);
    p[1] = uint8_t(n >> 16);
    p[2] = uint8_t(n >> 8);
    p[3] = uint8_t(n);
}
void le32(uint8_t *p, uint32_t n) {
    p[0] = uint8_t(n);
    p[1] = uint8_t(n >> 8);
    p[2] = uint8_t(n >> 16);
    p[3] = uint8_t(n >> 24);
}
size_t chunk(uint8_t *p, const char *type, const uint8_t *data, size_t n) {
    be32(p, uint32_t(n));
    memcpy(p + 4, type, 4);
    memcpy(p + 8, data, n);
    be32(p + 8 + n, crc32(p + 4, n + 4));
    return n + 12;
}
} // namespace
void render(const Snapshot &source, unsigned phase, uint8_t *frame) {
    if (!frame)
        return;
    Snapshot s = source;
    s.percent = std::min(100u, unsigned(s.percent));
    renderScene(s, phase, frame);
    print_layout::draw(s, phase, frame);
}
static void writePalette(uint8_t *out, uint32_t filamentColor, uint32_t feedColor) {
    for (unsigned index = 0; index < paletteCount; ++index) {
        uint32_t value = rgb(index, filamentColor, feedColor);
        out[index * 3] = value >> 16;
        out[index * 3 + 1] = value >> 8;
        out[index * 3 + 2] = value;
    }
}
size_t png(const uint8_t *f, uint8_t *out, size_t capacity, uint32_t filamentColor,
           uint32_t feedColor) {
    // 32 scanlines, one filter byte plus 32 palette indexes, one stored DEFLATE block.
    constexpr size_t raw = 33 * 32, compressed = raw + 11,
                     required = 8 + 25 + (paletteCount * 3 + 12) + 12 + compressed + 12;
    if (!f || !out || capacity < required)
        return 0;
    for (size_t i = 0; i < pixels; ++i)
        if (f[i] >= paletteCount)
            return 0;
    const uint8_t signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    memcpy(out, signature, 8);
    uint8_t ihdr[13] = {};
    be32(ihdr, 32);
    be32(ihdr + 4, 32);
    ihdr[8] = 8;
    ihdr[9] = 3;
    size_t pos = 8;
    pos += chunk(out + pos, "IHDR", ihdr, 13);
    be32(out + pos, paletteCount * 3);
    memcpy(out + pos + 4, "PLTE", 4);
    writePalette(out + pos + 8, filamentColor, feedColor);
    be32(out + pos + 8 + paletteCount * 3, crc32(out + pos + 4, paletteCount * 3 + 4));
    pos += paletteCount * 3 + 12;
    uint8_t *p = out + pos;
    be32(p, compressed);
    memcpy(p + 4, "IDAT", 4);
    uint8_t *z = p + 8;
    z[0] = 0x78;
    z[1] = 0x01;
    z[2] = 1;
    z[3] = uint8_t(raw);
    z[4] = uint8_t(raw >> 8);
    z[5] = uint8_t(~raw);
    z[6] = uint8_t((~raw) >> 8);
    uint32_t a = 1, b = 0;
    size_t index = 7;
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 33; ++x) {
            const uint8_t value = x ? f[y * 32 + x - 1] : 0;
            z[index++] = value;
            a = (a + value) % 65521;
            b = (b + a) % 65521;
        }
    be32(z + index, (b << 16) | a);
    be32(p + 8 + compressed, crc32(p + 4, compressed + 4));
    pos += compressed + 12;
    pos += chunk(out + pos, "IEND", out, 0);
    return pos;
}

size_t gif(const Snapshot &snapshot, unsigned frames, uint8_t *frame, uint8_t *output,
           size_t capacity, uint8_t *workspace, size_t workspaceCapacity) {
    if (!frame || !output || !frames || frames > 512 || capacity < 512)
        return 0;
    constexpr size_t workspaceBytes = pixels + 256 * sizeof(uint32_t) + 256;
    std::unique_ptr<uint8_t[]> owned;
    if (!workspace) {
        owned.reset(new (std::nothrow) uint8_t[workspaceBytes]);
        workspace = owned.get();
        workspaceCapacity = workspaceBytes;
    }
    if (!workspace || workspaceCapacity < workspaceBytes) return 0;
    uint8_t *previous = workspace;
    auto *keys = reinterpret_cast<uint32_t *>(workspace + pixels);
    uint8_t *values = workspace + pixels + 256 * sizeof(uint32_t);
    constexpr unsigned clear = 1u << paletteBits, end = clear + 1, codeBits = paletteBits + 1,
                       limit = (1u << codeBits) - 1;
    // Sparse rectangle updates and fixed-width bounded LZW keep full scroll cycles small.
    // Capacity fallback samples the complete cycle rather than dropping its later pages.
    for (unsigned stride = 1; stride <= 8; stride *= 2) {
        size_t pos = 0, delayAt = 0;
        bool okay = true, havePrevious = false;
        auto byte = [&](uint8_t v) {
            if (pos < capacity)
                output[pos++] = v;
            else
                okay = false;
        };
        auto word = [&](unsigned v) {
            byte(uint8_t(v));
            byte(uint8_t(v >> 8));
        };
        for (char c : {'G', 'I', 'F', '8', '9', 'a'})
            byte(uint8_t(c));
        word(32);
        word(32);
        byte(uint8_t(0xf0 | (paletteBits - 1)));
        byte(0);
        byte(0);
        for (unsigned i = 0; i < paletteCount; i++) {
            uint32_t c = rgb(i, snapshot.filamentColor, snapshot.feedColor);
            byte(c >> 16);
            byte(c >> 8);
            byte(c);
        }
        const uint8_t loop[] = {0x21, 0xff, 11,  'N', 'E', 'T', 'S', 'C', 'A', 'P',
                                'E',  '2',  '.', '0', 3,   1,   0,   0,   0};
        for (auto v : loop)
            byte(v);
        for (unsigned phase = 0; phase < frames && okay; phase += stride) {
            render(snapshot, (snapshot.stateAgeMs / print_layout::frameMs + phase) % frames, frame);
            int left = 32, top = 32, right = -1, bottom = -1;
            for (int y = 0; y < 32; y++)
                for (int x = 0; x < 32; x++)
                    if (!havePrevious || frame[y * 32 + x] != previous[y * 32 + x]) {
                        left = std::min(left, x);
                        top = std::min(top, y);
                        right = std::max(right, x);
                        bottom = std::max(bottom, y);
                    }
            unsigned delay = (print_layout::frameMs / 10) * std::min(stride, frames - phase);
            if (right < left) {
                unsigned old = unsigned(output[delayAt]) | unsigned(output[delayAt + 1]) << 8;
                old += delay;
                output[delayAt] = uint8_t(old);
                output[delayAt + 1] = uint8_t(old >> 8);
                continue;
            }
            byte(0x21);
            byte(0xf9);
            byte(4);
            byte(4);
            delayAt = pos;
            word(delay);
            byte(0);
            byte(0);
            byte(0x2c);
            word(unsigned(left));
            word(unsigned(top));
            word(unsigned(right - left + 1));
            word(unsigned(bottom - top + 1));
            byte(0);
            byte(paletteBits);
            size_t blockAt = pos;
            byte(0);
            unsigned length = 0, pending = 0;
            uint32_t accumulator = 0;
            auto packedByte = [&](uint8_t v) {
                if (length == 255) {
                    if (blockAt < capacity)
                        output[blockAt] = 255;
                    blockAt = pos;
                    byte(0);
                    length = 0;
                }
                byte(v);
                ++length;
            };
            auto code = [&](unsigned v) {
                accumulator |= uint32_t(v) << pending;
                pending += codeBits;
                while (pending >= 8) {
                    packedByte(uint8_t(accumulator));
                    accumulator >>= 8;
                    pending -= 8;
                }
            };
            auto reset = [&]() { std::memset(keys, 0xff, 256 * sizeof(uint32_t)); };
            reset();
            unsigned next = clear + 2, prefix = 0;
            bool first = true;
            code(clear);
            for (int y = top; y <= bottom && okay; y++)
                for (int x = left; x <= right && okay; x++) {
                    unsigned value = frame[y * 32 + x];
                    if (value >= paletteCount) {
                        okay = false;
                        break;
                    }
                    if (first) {
                        prefix = value;
                        first = false;
                        continue;
                    }
                    uint32_t key = (prefix << 8) | value;
                    unsigned slot = (key * 2654435761u) >> 24;
                    while (keys[slot] != 0xffffffffu && keys[slot] != key)
                        slot = (slot + 1) & 255;
                    if (keys[slot] == key) {
                        prefix = values[slot];
                        continue;
                    }
                    code(prefix);
                    keys[slot] = key;
                    values[slot] = uint8_t(next++);
                    prefix = value;
                    if (next == limit) {
                        code(clear);
                        reset();
                        next = clear + 2;
                    }
                }
            code(prefix);
            code(end);
            if (pending)
                packedByte(uint8_t(accumulator));
            if (blockAt < capacity)
                output[blockAt] = uint8_t(length);
            byte(0);
            std::memcpy(previous, frame, pixels);
            havePrevious = true;
        }
        byte(0x3b);
        if (okay)
            return pos;
    }
    return 0;
}

bool upload(Format format, const uint8_t *image, size_t size, uint8_t *scratch, size_t capacity,
            Sink sink, void *context) {
    if (!image || !size || !scratch || !sink || size > 65535)
        return false;
    const size_t header = format == Format::Gif ? 16 : 9;
    const size_t block = 4096;
    if (capacity < header + (size < block ? size : block))
        return false;
    // PNG's length includes one continuation marker per block and is a 16-bit field.
    const size_t chunks = (size + block - 1) / block;
    if (format == Format::Png && size + chunks > 65535)
        return false;
    const uint32_t crc = format == Format::Gif ? crc32(image, size) : 0;
    for (size_t offset = 0; offset < size;) {
        const size_t n = size - offset < block ? size - offset : block;
        memset(scratch, 0, header);
        const size_t length = format == Format::Gif ? n + header : size + chunks;
        scratch[0] = uint8_t(length);
        scratch[1] = uint8_t(length >> 8);
        scratch[2] = format == Format::Gif ? 1 : 0;
        scratch[4] = offset ? 2 : 0;
        le32(scratch + 5, uint32_t(size));
        if (format == Format::Gif) {
            le32(scratch + 9, crc);
            scratch[13] = 5;
            scratch[15] = 13;
        }
        memcpy(scratch + header, image + offset, n);
        if (!sink(scratch, n + header, context))
            return false;
        offset += n;
    }
    return true;
}
} // namespace pixel_screen
