#pragma once
#include "xtouch/types.h"
#include <string.h>
inline bool xtouch_deep_idle_reported(const char *state){
 return !strcmp(state,"IDLE")||!strcmp(state,"FINISH")||!strcmp(state,"FAILED");
}
inline bool xtouch_print_wake_requested(const char *previous,const char *current,bool enabled){
 return enabled&&strcmp(previous,current)&&(!strcmp(current,"RUNNING")||!strcmp(current,"PREPARE")||!strcmp(current,"PAUSE"));
}
inline bool xtouch_deep_idle_allowed(int state,bool online,bool fault,bool maintenance,bool transferBusy){
 return online&&!fault&&!maintenance&&!transferBusy&&
  (state==XTOUCH_PRINT_STATUS_IDLE||state==XTOUCH_PRINT_STATUS_FINISHED||state==XTOUCH_PRINT_STATUS_FAILED);
}
