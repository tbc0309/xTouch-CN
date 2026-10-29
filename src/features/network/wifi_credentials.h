#ifndef XTOUCH_WIFI_CREDENTIALS_H
#define XTOUCH_WIFI_CREDENTIALS_H

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace xtouch {
namespace network {

inline bool credentialLength(const char *text, std::size_t capacity, std::size_t &length) {
    std::size_t count = 0;
    if (text) {
        while (count <= capacity && text[count]) {
            ++count;
        }
    }
    if (count > capacity) {
        return false;
    }
    length = count;
    return true;
}

// IDF credential fields are byte arrays: a full-width value needs no trailing NUL.
// Reject overflow before changing either destination or the reported SSID length.
template <std::size_t SsidSize, std::size_t PasswordSize>
bool fillWifiCredentials(uint8_t (&ssidField)[SsidSize], uint8_t (&passwordField)[PasswordSize],
                         const char *ssid, const char *password, std::size_t &ssidLength) {
    std::size_t ssidBytes, passwordBytes;
    if (!credentialLength(ssid, SsidSize, ssidBytes) || !ssidBytes ||
        !credentialLength(password, PasswordSize, passwordBytes)) {
        return false;
    }
    std::memset(ssidField, 0, SsidSize);
    std::memset(passwordField, 0, PasswordSize);
    std::memcpy(ssidField, ssid, ssidBytes);
    if (passwordBytes) {
        std::memcpy(passwordField, password, passwordBytes);
    }
    ssidLength = ssidBytes;
    return true;
}

} // namespace network
} // namespace xtouch

#endif
