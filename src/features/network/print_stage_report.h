#pragma once
#include <ArduinoJson.h>
#include <cmath>
#include <cstdlib>

namespace XTouchStageReport {
inline int read(JsonVariantConst value) {
    double number;
    if (value.is<const char *>()) {
        const char *text = value.as<const char *>();
        if (!text || !*text)
            return -1;
        char *end;
        number = std::strtod(text, &end);
        if (*end)
            return -1;
    } else if (!value.is<bool>() && value.is<double>()) {
        number = value.as<double>();
    } else
        return -1;
    if (!std::isfinite(number) || std::floor(number) != number || number < 0 || number > 65535 ||
        number == 255)
        return -1;
    return int(number);
}
inline int update(JsonObjectConst report, int previous, bool newJob, bool firstLayerStarted,
                  bool terminal) {
    if (terminal)
        return -1;
    // Reports are deltas; do not carry preparation across task/layer boundaries.
    if (newJob || firstLayerStarted)
        previous = -1;
    return report.containsKey("stg_cur") ? read(report["stg_cur"]) : previous;
}
} // namespace XTouchStageReport
