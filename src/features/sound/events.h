#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { XTOUCH_SOUND_TOUCH, XTOUCH_SOUND_WARNING, XTOUCH_SOUND_ERROR, XTOUCH_SOUND_COMPLETE } XTouchSoundEvent;
typedef void (*XTouchSoundSink)(XTouchSoundEvent);
void xtouch_sound_bind(XTouchSoundSink sink);
void xtouch_sound_preferences(bool touch,bool warning);
void xtouch_sound_event(XTouchSoundEvent event,uint32_t now);
#ifdef __cplusplus
}
void xtouch_sound_begin();
bool xtouch_sound_test(XTouchSoundEvent event);
void xtouch_sound_suspend(bool sleeping);
struct XTouchSoundStatus {bool available,playing,sleeping;unsigned played,dropped;int error;};
XTouchSoundStatus xtouch_sound_status();
#endif
