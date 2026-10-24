#pragma once
#include <stdint.h>
#include <string.h>
#include <ctype.h>
/* A missing result is an echo, not evidence that an actuator accepted a request. */
enum XtouchReplyResult { XTOUCH_REPLY_WAIT, XTOUCH_REPLY_ACCEPTED, XTOUCH_REPLY_REJECTED };
static inline bool xtouch_ascii_equal(const char *a,const char *b){
 if(!a||!b)return false;while(*a&&*b){if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return false;}return !*a&&!*b;
}
static inline bool xtouch_ascii_contains(const char *text,const char *part){
 if(!text||!part)return false;for(;*text;text++){const char *a=text,*b=part;while(*a&&*b&&tolower((unsigned char)*a)==tolower((unsigned char)*b)){a++;b++;}if(!*b)return true;}return false;
}
static inline XtouchReplyResult xtouch_reply_result(const char *result,uint32_t error){
 if(error)return XTOUCH_REPLY_REJECTED;
 if(xtouch_ascii_equal(result,"success")||xtouch_ascii_equal(result,"ok"))return XTOUCH_REPLY_ACCEPTED;
 if(xtouch_ascii_equal(result,"fail")||xtouch_ascii_equal(result,"failed")||xtouch_ascii_equal(result,"error"))return XTOUCH_REPLY_REJECTED;
 return XTOUCH_REPLY_WAIT;
}
static inline bool xtouch_reply_auth_failure(const char *reason,uint32_t error){
 return error==84033543UL||xtouch_ascii_contains(reason,"verify failed")||xtouch_ascii_contains(reason,"unauthorized")||xtouch_ascii_contains(reason,"signature");
}
static inline bool xtouch_rfid_legacy_allowed(const char *model,int tray,bool idle,bool unloaded,const char *reason,uint32_t error){
 if(!model||tray<0||tray>=16||!idle||!unloaded||xtouch_reply_auth_failure(reason,error))return false;
 const char *models[]={"BL-P001","BL-P002","C13","C11","C12","N1","N2S"};bool old=false;
 for(const char *m:models)if(!strcmp(model,m))old=true;
 // Only an explicit protocol refusal; never retry arbitrary busy/failure responses.
 return old&&(xtouch_ascii_equal(reason,"ERROR STATE")||xtouch_ascii_contains(reason,"not support")||xtouch_ascii_contains(reason,"unknown command"));
}
