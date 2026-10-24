#include "indicator.h"
#include "features/config.h"
#include <Arduino.h>
bool xtouch_indicator_available(){
#if XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED
 return true;
#else
 // S3/P4 power/charge LEDs are hardware-wired, not general GPIO status LEDs.
 return false;
#endif
}
void xtouch_indicator_begin(){
#if XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED
 for(int pin:{4,16,17}){pinMode(pin,OUTPUT);digitalWrite(pin,HIGH);}
#endif
}
void xtouch_indicator_poll(bool connected,bool warning,bool sleeping,bool enabled){
#if XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED
 // Active-low RGB: quiet green online, slow blue reconnect, amber fault.
 bool on=enabled&&!sleeping;bool blink=(millis()%1600)<400;
 unsigned mask=(!(on&&warning&&blink))|((!(on&&(connected||warning)))<<1)|((!(on&&!connected&&!warning&&blink))<<2);
 static unsigned last=255;if(last==mask)return;last=mask;
 digitalWrite(4,mask&1);digitalWrite(16,(mask>>1)&1);digitalWrite(17,(mask>>2)&1);
#else
 (void)connected;(void)warning;(void)sleeping;(void)enabled;
#endif
}
