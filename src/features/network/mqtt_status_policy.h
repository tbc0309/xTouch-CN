#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t initialQueryAt;
    bool modelRequested;
    bool expired;
    bool received;
} XTouchMqttStatusPolicy;

/* Studio separates a 15 s push-info freshness window from its 30 s LAN
 * disconnect window. Idle P1 reports can pause longer than the former. */
static inline uint32_t xtouch_mqtt_status_timeout(bool cloud) {
    return cloud ? 180000u : 30000u;
}

static inline void xtouch_mqtt_status_connected(XTouchMqttStatusPolicy *p, uint32_t now) {
    p->initialQueryAt = now;
    p->modelRequested = false;
    p->expired = false;
    p->received = false;
}
static inline bool xtouch_mqtt_status_model_due(XTouchMqttStatusPolicy *p, uint32_t now,
                                                bool received) {
    if (!received || p->modelRequested || (uint32_t)(now - p->initialQueryAt) < 5000u)
        return false;
    p->modelRequested = true;
    return true;
}
static inline bool xtouch_mqtt_status_expired(XTouchMqttStatusPolicy *p, uint32_t now,
                                              uint32_t last, uint32_t timeout) {
    if (p->expired || (uint32_t)(now - last) <= timeout)
        return false;
    p->expired = true;
    return true;
}
static inline void xtouch_mqtt_status_received(XTouchMqttStatusPolicy *p) {
    p->expired = false;
    p->received = true;
}
