#include "events.h"
static XTouchSoundSink sink;
static bool touchEnabled,warningEnabled;
static uint32_t lastTouch,lastWarning;
static bool touched,warned;
void xtouch_sound_bind(XTouchSoundSink value){sink=value;}
void xtouch_sound_preferences(bool touch,bool warning){touchEnabled=touch;warningEnabled=warning;}
void xtouch_sound_event(XTouchSoundEvent event,uint32_t now){
 if(!sink)return;
 if(event==XTOUCH_SOUND_TOUCH){if(!touchEnabled||(touched&&(uint32_t)(now-lastTouch)<70))return;touched=true;lastTouch=now;}
 else {if(!warningEnabled||(warned&&(uint32_t)(now-lastWarning)<5000))return;warned=true;lastWarning=now;}
 sink(event);
}
