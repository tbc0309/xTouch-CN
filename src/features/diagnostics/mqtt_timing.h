#pragma once
#include <stdint.h>

// Durations only: never retain broker addresses, credentials or packet contents.
struct XTouchMqttTiming {
    uint32_t attempts, transportMs, loginMs, subscribeMs, totalMs;
    unsigned cpuBeforeMHz, cpuWorkMHz;
    int state;
    bool transportReady, connected;
};
extern XTouchMqttTiming xtouch_mqtt_timing;

// Live report state is separate from the previous connection-attempt timings.
struct XTouchMqttReportSnapshot {
    bool received, expired;
    uint32_t ageMs;
};
XTouchMqttReportSnapshot xtouch_mqtt_report_snapshot();
