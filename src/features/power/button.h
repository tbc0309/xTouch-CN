#pragma once
#include <stdint.h>
/* One action on a debounced release. Ignore a key held during boot. */
class XTouchSleepButton {
 bool raw=false,stable=false,armed=false,pressedAfterArming=false;
 uint32_t changed=0;
public:
 void begin(bool pressed,uint32_t now){raw=stable=pressed;armed=!pressed;pressedAfterArming=false;changed=now;}
 bool update(bool pressed,uint32_t now){
  if(pressed!=raw){raw=pressed;changed=now;}
  if(uint32_t(now-changed)<35||stable==raw)return false;
  stable=raw;
  if(stable){pressedAfterArming=armed;return false;}
  bool fire=armed&&pressedAfterArming;armed=true;pressedAfterArming=false;return fire;
 }
};
