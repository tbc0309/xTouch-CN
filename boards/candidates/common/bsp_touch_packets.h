#pragma once
#include <cstddef>
#include <cstdint>
namespace CandidateTouch {
enum class Event { Ignore, Released, Pressed };
// TT21100 HID reports have a seven-byte header and ten bytes per contact.
inline Event tt21100(const uint8_t *data, size_t length, int &x, int &y) {
    if (!data || length < 7 || length > 57 ||
        (static_cast<unsigned>(data[0]) | static_cast<unsigned>(data[1]) << 8) != length ||
        data[2] == 3)
        return Event::Ignore;
    const unsigned count = data[5] & 31;
    if (count > 5 || length != 7 + count * 10)
        return Event::Ignore;
    if (!count)
        return Event::Released;
    for (unsigned n = 0; n < count; ++n) {
        const uint8_t *point = data + 7 + n * 10;
        if ((point[0] & 7) != 0 || (point[1] & 0x80) == 0)
            continue;
        x = static_cast<unsigned>(point[2]) | static_cast<unsigned>(point[3]) << 8;
        y = static_cast<unsigned>(point[4]) | static_cast<unsigned>(point[5]) << 8;
        return Event::Pressed;
    }
    return Event::Released;
}
} // namespace CandidateTouch
