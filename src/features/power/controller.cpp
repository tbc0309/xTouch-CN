#include "controller.h"
#include "deep_sleep.h"
#include "button.h"
#include "runtime.h"
#include "features/config.h"
#include "platform/idf/runtime.h"
#include "xtouch/portal.h"
#include "xtouch/types.h"
extern void xtouch_screen_sleep();
extern void xtouch_screen_wakeUp();
static unsigned pendingMode;
static bool safeLast;
bool xtouch_display_request(unsigned mode){
 if(mode<1||mode>3||pendingMode)return false;
 if(mode==3&&(!xtouch_deep_sleep_available()||!safeLast))return false;
 pendingMode=mode;return true;
}
void xtouch_display_poll(bool safeIdle){
#if XTOUCH_SLEEP_BUTTON_PIN>=0
 static XTouchSleepButton key;static bool initialized;
 if(!initialized){pinMode(XTOUCH_SLEEP_BUTTON_PIN,INPUT_PULLUP);key.begin(digitalRead(XTOUCH_SLEEP_BUTTON_PIN)==LOW,millis());initialized=true;}
 if(xtouch_portal_maintenance())key.begin(digitalRead(XTOUCH_SLEEP_BUTTON_PIN)==LOW,millis());
 else if(key.update(digitalRead(XTOUCH_SLEEP_BUTTON_PIN)==LOW,millis())){
  unsigned mode=xtouch_power_status().screenSleeping?2:1;
  if(xtouch_display_request(mode))Serial.printf("[POWER] key GPIO%d requested %s\n",XTOUCH_SLEEP_BUTTON_PIN,mode==1?"sleep":"wake");
 }
#endif
 safeLast=safeIdle;unsigned mode=pendingMode;pendingMode=0;
 if(mode==1)xtouch_screen_sleep();
 else if(mode==2)xtouch_screen_wakeUp();
 else if(mode==3&&safeIdle){xtouch_screen_sleep();xtouch_deep_sleep_request();}
 xtouch_deep_sleep_poll(safeIdle);
}
