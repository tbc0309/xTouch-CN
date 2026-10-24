#pragma once
#include <Arduino.h>
enum XTouchLoopPhase {XTOUCH_PHASE_GUI,XTOUCH_PHASE_PORTAL,XTOUCH_PHASE_MQTT,XTOUCH_PHASE_CONNECT,XTOUCH_PHASE_RECEIVE,XTOUCH_PHASE_PREVIEW,XTOUCH_PHASE_SD,XTOUCH_PHASE_COUNT};
struct XTouchLoopTiming {uint32_t maximum[XTOUCH_PHASE_COUNT],lastMs,lastAt;unsigned lastPhase;};
extern XTouchLoopTiming xtouch_loop_timing;
struct XTouchLoopScope {
 XTouchLoopPhase phase;uint32_t started;
 explicit XTouchLoopScope(XTouchLoopPhase p):phase(p),started(millis()){}
 ~XTouchLoopScope(){uint32_t elapsed=millis()-started;if(elapsed>xtouch_loop_timing.maximum[phase])xtouch_loop_timing.maximum[phase]=elapsed;if(elapsed>=100){xtouch_loop_timing.lastMs=elapsed;xtouch_loop_timing.lastAt=millis();xtouch_loop_timing.lastPhase=phase;}}
};
