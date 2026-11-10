#pragma once
#include "ota_selection.h"
#include <ArduinoJson.h>
#include <cstring>
#include <cctype>
namespace XTouchOtaSelection {
inline bool imageUrl(const char *image, const char *version, const char *filename,
                     const char *repository = "tbc0309/xTouch-CN") {
    constexpr char prefix[] = "https://github.com/";
    constexpr char download[] = "/releases/download/";
    if (!image || !version || !filename || !repository || filename[0] != '/')
        return false;
    const size_t base = sizeof(prefix) - 1, repo = std::strlen(repository),
                 dir = sizeof(download) - 1;
    const size_t tag = std::strlen(version), file = std::strlen(filename);
    return std::strlen(image) == base + repo + dir + tag + file &&
           !std::strncmp(image, prefix, base) && !std::strncmp(image + base, repository, repo) &&
           !std::strncmp(image + base + repo, download, dir) &&
           !std::strncmp(image + base + repo + dir, version, tag) &&
           !std::strcmp(image + base + repo + dir + tag, filename);
}

inline bool manifest(const char *body, const char *filename, uint32_t limit, Version &version,
                     bool allowTest = false, const char *repository = "tbc0309/xTouch-CN") {
    DynamicJsonDocument doc(2048);
    if (!body || !filename || deserializeJson(doc, body))
        return false;
    const char *digest = doc["md5"] | "", *image = doc["url"] | "";
    if (!parse(doc["version"] | "", version) || (version.test && !allowTest) ||
        std::strlen(digest) != 32 || !doc["size"].is<uint32_t>())
        return false;
    for (unsigned i = 0; i < 32; ++i)
        if (!std::isxdigit(static_cast<unsigned char>(digest[i])))
            return false;
    return doc["size"].as<uint32_t>() > 0 && doc["size"].as<uint32_t>() <= limit &&
           imageUrl(image, doc["version"] | "", filename, repository);
}
template <class Body, size_t N, class Fetch>
bool choose(const char *const (&paths)[N], Fetch fetch, const char *filename, uint32_t limit,
            Body &selectedBody, bool allowTest = false,
            const char *repository = "tbc0309/xTouch-CN") {
    selectedBody = "";
    bool found = false;
    Version selected{};
    for (const char *path : paths) {
        Body candidate;
        Version version{};
        if (!fetch(path, candidate) ||
            !manifest(candidate.c_str(), filename, limit, version, allowTest, repository))
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
