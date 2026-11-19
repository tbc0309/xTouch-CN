#pragma once
#include <atomic>
#include <cstdint>
#include <climits>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

namespace XTouchStackMargin {
enum class Path : unsigned { CloudPreview, LocalPreview, H264, Jpeg, OtaHttps, OtaSd, Count };
inline const char *name(Path path) {
    static constexpr const char *names[] = {"cloudPreview", "localPreview", "h264",
                                            "jpeg",         "otaHttps",     "otaSd"};
    return names[unsigned(path)];
}
struct Counter {
    std::atomic<unsigned> minimum{UINT_MAX}, samples{0}, lastAt{0};
    std::atomic<bool> warned{false}, reported{false};
};
inline Counter &counter(Path path) {
    static Counter counters[unsigned(Path::Count)];
    return counters[unsigned(path)];
}
inline void sample(Path path, bool force = false) {
    auto &counter = XTouchStackMargin::counter(path);
    unsigned now = unsigned(xTaskGetTickCount());
    if (!force && counter.samples.load() &&
        unsigned(now - counter.lastAt.load()) < pdMS_TO_TICKS(1000))
        return;
    counter.lastAt = now;
    // ESP-IDF returns bytes here. The high-water value covers this task's
    // lifetime, including SDK calls that returned before this checkpoint.
    unsigned free = uxTaskGetStackHighWaterMark(nullptr), previous = counter.minimum.load();
    while (free < previous && !counter.minimum.compare_exchange_weak(previous, free)) {
    }
    ++counter.samples;
    // Do not format logs on the task whose stack is nearly exhausted.
    if (free < 1024)
        counter.warned = true;
}
inline void logPendingWarnings() {
    // Diagnostics runs on the main task. Retain the API flag if this task also
    // has little historical margin, rather than spending stack on a log call.
    if (uxTaskGetStackHighWaterMark(nullptr) < 2048)
        return;
    for (unsigned i = 0; i < unsigned(Path::Count); ++i) {
        auto path = static_cast<Path>(i);
        auto &entry = counter(path);
        if (entry.warned.load() && !entry.reported.exchange(true))
            ESP_LOGW("stack", "%s task minimum remaining=%u bytes", name(path),
                     entry.minimum.load());
    }
}
struct Scope {
    Path path;
    explicit Scope(Path value) : path(value) {
        sample(path, true);
    }
    ~Scope() {
        sample(path, true);
    }
};
} // namespace XTouchStackMargin
