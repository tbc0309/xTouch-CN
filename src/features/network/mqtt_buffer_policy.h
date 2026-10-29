#pragma once
#include <stddef.h>
#include <initializer_list>

namespace XTouchMqttBuffers {
constexpr size_t MinimumOutput = 1024;
constexpr size_t MaximumOutput = 4096;
constexpr size_t ConnectOverhead = 64;

// CONNECT must fit credentials as well as MQTT framing. Larger queued PUBLISH
// payloads are fragmented by esp-mqtt; this is not a packet-size limit.
inline size_t output(size_t clientIdBytes, size_t usernameBytes, size_t passwordBytes) {
    size_t bytes = ConnectOverhead;
    for (size_t part : {clientIdBytes, usernameBytes, passwordBytes}) {
        if (part > MaximumOutput - bytes)
            return 0;
        bytes += part;
    }
    if (bytes <= MinimumOutput)
        return MinimumOutput;
    return (bytes + 63) & ~size_t(63);
}
} // namespace XTouchMqttBuffers
