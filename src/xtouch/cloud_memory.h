#pragma once
#include <ArduinoJson.h>
#include <utility>
namespace XTouchCloudMemory {
// Keep the output pool absent during TLS, then initialize the root before moving
// the pool. Moving an empty ArduinoJson6 document can copy an uninitialized union.
inline void output(DynamicJsonDocument &document, size_t capacity = 4096) {
    DynamicJsonDocument allocated(capacity);
    allocated.to<JsonObject>();
    document = std::move(allocated);
}
} // namespace XTouchCloudMemory
