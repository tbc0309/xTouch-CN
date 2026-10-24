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
