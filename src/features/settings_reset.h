#pragma once

namespace xtouch {
namespace settings {
enum class ResetResult { success, busy, key_failed, storage_failed };

// Keep removable configuration and transaction copies in one reset policy.
template <typename Rotate, typename Remove>
ResetResult reset(bool busy, Rotate rotate, Remove remove) {
    if (busy)
        return ResetResult::busy;
    if (!rotate())
        return ResetResult::key_failed;
    constexpr const char *paths[] = {"/xtouch.json",       "/xtouch/settings.json",
                                     "/xtouch/touch.json", "/xtouch/hardware.json",
                                     "/xtouch/admin.json", "/xtouch/env.json"};
    constexpr const char *suffixes[] = {"", ".tmp", ".previous"};
    bool complete = true;
    for (const char *path : paths) {
        for (const char *suffix : suffixes) {
            if (!remove(path, suffix))
                complete = false;
        }
    }
    return complete ? ResetResult::success : ResetResult::storage_failed;
}
} // namespace settings
} // namespace xtouch
