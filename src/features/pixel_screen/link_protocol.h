#pragma once
#include <cstddef>
#include <cstdint>
namespace pixel_screen {
namespace link {
enum class Reply : uint8_t { Ignore, Accepted, Rejected, Next, Done, NoSpace };
struct DeviceInfo {
    uint8_t major, minor, screenType;
    bool passwordProtected;
};
size_t clockMode(uint8_t *out);
bool passwordValid(const char *password);
size_t verifyPassword(const char *password, uint8_t *out);
size_t resetDevice(uint8_t *out);
// Deletes the twelve stored artwork slots; not the live GIF cache.
size_t clearMaterials(uint8_t *out);
size_t restorePassword(const char *password, uint8_t *out);
bool deviceInfo(const uint8_t *data, size_t length, DeviceInfo &out);
Reply commandReply(const uint8_t *data, size_t length, uint8_t command, uint8_t sub);
Reply uploadReply(const uint8_t *data, size_t length);
size_t writeSize(unsigned mtu);
uint32_t retryDelay(unsigned failures);
uint32_t uploadGapMs(unsigned intervalSeconds, bool stateChanged, bool detailOnly = false);
bool uploadReady(bool sent, bool changed, bool stateChanged, unsigned interval, uint32_t now,
                 uint32_t lastFinished, bool detailOnly = false);
} // namespace link
} // namespace pixel_screen
