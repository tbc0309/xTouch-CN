#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t initialQueryAt;
    bool modelRequested;
    bool expired;
} XTouchMqttStatusPolicy;

static inline void xtouch_mqtt_status_connected(XTouchMqttStatusPolicy *p, uint32_t now)
{
    p->initialQueryAt=now;p->modelRequested=false;p->expired=false;
}
static inline bool xtouch_mqtt_status_model_due(XTouchMqttStatusPolicy *p, uint32_t now, bool received)
{
    if(!received||p->modelRequested||(uint32_t)(now-p->initialQueryAt)<5000u)return false;
    p->modelRequested=true;return true;
}
static inline bool xtouch_mqtt_status_expired(XTouchMqttStatusPolicy *p, uint32_t now, uint32_t last, uint32_t timeout)
{
    if(p->expired||(uint32_t)(now-last)<=timeout)return false;
    p->expired=true;return true;
}
static inline void xtouch_mqtt_status_received(XTouchMqttStatusPolicy *p){p->expired=false;}
