#pragma once
#include <stdint.h>
struct XTouchScreenProtection {uint8_t brightness;int8_t x,y;bool standby;};
inline XTouchScreenProtection xtouch_screen_protection(uint32_t idle,uint8_t requested,bool asleep,bool liveVideo,bool enabled){
 XTouchScreenProtection p={uint8_t(asleep?0:requested),0,0,false};
 if(!enabled||asleep||liveVideo)return p;
 if(idle>=300000){p.brightness=0;p.standby=true;return p;}
 if(idle>=60000){
  static const int8_t positions[8][2]={{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}};
  unsigned phase=(idle/60000-1)%8;p.x=positions[phase][0];p.y=positions[phase][1];
 }
 if(idle>=120000){uint8_t dim=requested/3;if(dim<8)dim=8;if(dim>requested)dim=requested;p.brightness=dim;}
 return p;
}
