#include "link_protocol.h"
#include <cstring>
namespace pixel_screen {
namespace link {
size_t clockMode(uint8_t *out) {
    if (!out) return 0;
    const uint8_t command[] = {8, 0, 6, 1, 0x40, 255, 255, 255};
    memcpy(out, command, sizeof(command));
    return sizeof(command);
}
bool passwordValid(const char *p) {
    if (!p || strlen(p) != 6)
        return false;
    for (unsigned i = 0; i < 6; ++i)
        if (p[i] < '0' || p[i] > '9')
            return false;
    return true;
}
size_t verifyPassword(const char *p, uint8_t *out) {
    if (!out || !passwordValid(p))
        return 0;
    out[0] = 7;
    out[1] = 0;
    out[2] = 5;
    out[3] = 2;
    for (unsigned i = 0; i < 3; ++i)
        out[4 + i] = (p[2 * i] - '0') * 10 + p[2 * i + 1] - '0';
    return 7;
}
size_t clearMaterials(uint8_t *out) {
    if (!out) return 0;
    const uint8_t command[] = {17, 0, 2, 1, 12, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    memcpy(out, command, sizeof(command));
    return sizeof(command);
}
size_t resetDevice(uint8_t *out) {
    if (!out) return 0;
    const uint8_t command[] = {4, 0, 3, 0x80};
    memcpy(out, command, sizeof(command));
    return sizeof(command);
}
size_t restorePassword(const char *p, uint8_t *out) {
    if (!out || !passwordValid(p)) return 0;
    out[0] = 8; out[1] = 0; out[2] = 4; out[3] = 2; out[4] = 1;
    for (unsigned i = 0; i < 3; ++i)
        out[5 + i] = (p[2 * i] - '0') * 10 + p[2 * i + 1] - '0';
    return 8;
}
bool deviceInfo(const uint8_t *d, size_t n, DeviceInfo &out) {
    if (!d || n < 9 || n > 32 || d[0] != n || d[1] || d[2] != 1 || d[3] != 0x80 || d[8] > 1)
        return false;
    out = {d[4], d[5], d[7], d[8] == 1};
    return true;
}
Reply commandReply(const uint8_t *d, size_t n, uint8_t cmd, uint8_t sub) {
    if (!d || n != 5 || d[0] != 5 || d[1] || d[2] != cmd || d[3] != sub)
        return Reply::Ignore;
    return d[4] == 1 ? Reply::Accepted : d[4] == 0 ? Reply::Rejected : Reply::Ignore;
}
Reply uploadReply(const uint8_t *d, size_t n) {
    if (!d || n != 5 || d[0] != 5 || d[1] || d[2] != 1 || d[3])
        return Reply::Ignore;
    switch (d[4]) {
    case 1:
        return Reply::Next;
    case 3:
        return Reply::Done;
    case 0:
        return Reply::Rejected;
    case 2:
        return Reply::NoSpace;
    default:
        return Reply::Ignore;
    }
}
size_t writeSize(unsigned mtu) {
    return mtu > 23 ? (mtu - 3 < 509 ? mtu - 3 : 509) : 18;
}
uint32_t uploadGapMs(unsigned intervalSeconds, bool stateChanged, bool detailOnly) {
    if (stateChanged) return 1000;
    const uint32_t normalGap = intervalSeconds < 5 ? 5000u : intervalSeconds * 1000;
    return detailOnly && normalGap < 10000u ? 10000u : normalGap;
}
bool uploadReady(bool sent, bool changed, bool stateChanged, unsigned interval, uint32_t now,
                 uint32_t lastFinished, bool detailOnly) {
    if (!sent) return true;
    if (!changed) return false;
    // Animation runs locally; coalesce telemetry without delaying a new scene.
    return uint32_t(now - lastFinished) >= uploadGapMs(interval, stateChanged, detailOnly);
}
uint32_t retryDelay(unsigned n) {
    uint32_t delay = 2000;
    while (n-- && delay < 60000)
        delay = delay > 30000 ? 60000 : delay * 2;
    return delay;
}
} // namespace link
} // namespace pixel_screen
