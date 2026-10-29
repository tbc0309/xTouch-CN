#pragma once
#include <cstddef>
#include <cstdint>

namespace XTouchPort {
enum class TouchPacket { Ignore, Released, Pressed };

// GT1151 reports use Status + 8*N point bytes + Key + additive checksum.
// Unlike GT911, its documented driver does not require a status bit-7 gate.
inline TouchPacket gt1151Packet(const uint8_t *data, size_t size, int &x, int &y) {
    if (!data || size == 0)
        return TouchPacket::Ignore;
    const unsigned count = data[0] & 15;
    if (count > 10)
        return TouchPacket::Ignore;
    if (count == 0)
        return TouchPacket::Released;
    const size_t length = 3 + count * 8;
    if (size < length)
        return TouchPacket::Ignore;
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; ++i)
        checksum += data[i];
    if (checksum != 0)
        return TouchPacket::Ignore;
    x = data[2] | unsigned(data[3]) << 8;
    y = data[4] | unsigned(data[5]) << 8;
    return TouchPacket::Pressed;
}
} // namespace XTouchPort
