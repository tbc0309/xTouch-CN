#pragma once
#include "ota_selection.h"
#include <ArduinoJson.h>
#include <cstring>
#include <cctype>
namespace XTouchOtaSelection {
inline bool imageUrl(const char *image, const char *version, const char *filename) {
    constexpr char prefix[] = "https://github.com/tbc0309/xTouch-CN/releases/download/";
    if (!image || !version || !filename || filename[0] != '/')
        return false;
    const auto base = sizeof(prefix) - 1, tag = std::strlen(version), file = std::strlen(filename);
    return std::strlen(image) == base + tag + file && !std::strncmp(image, prefix, base) &&
           !std::strncmp(image + base, version, tag) && !std::strcmp(image + base + tag, filename);
}
inline bool manifest(const char *body, const char *filename, uint32_t limit, Version &version) {
    DynamicJsonDocument doc(2048);
    if (!body || !filename || deserializeJson(doc, body))
        return false;
    const char *digest = doc["md5"] | "", *image = doc["url"] | "";
    if (!parse(doc["version"] | "", version) || std::strlen(digest) != 32 ||
        !doc["size"].is<uint32_t>())
        return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!std::isxdigit(static_cast<unsigned char>(digest[i])))
            return false;
    return doc["size"].as<uint32_t>() > 0 && doc["size"].as<uint32_t>() <= limit &&
           imageUrl(image, doc["version"] | "", filename);
}
template <class Body, size_t N, class Fetch>
bool choose(const char *const (&paths)[N], Fetch fetch, const char *filename, uint32_t limit,
            Body &selectedBody) {
    selectedBody = "";
    bool found = false;
    Version selected{};
    for (const char *path : paths) {
        Body candidate;
        Version version{};
        if (!fetch(path, candidate) || !manifest(candidate.c_str(), filename, limit, version))
            continue;
        // The caller lists main first. Equal test versions do not displace main.
        if (!found || compare(version, selected) > 0) {
            selectedBody = candidate;
            selected = version;
            found = true;
        }
    }
    return found;
}
} // namespace XTouchOtaSelection
