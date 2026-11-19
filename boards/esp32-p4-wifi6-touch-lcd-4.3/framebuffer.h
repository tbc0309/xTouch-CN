#pragma once
#include "../common/portrait_frame.h"
namespace P4Frame {
inline void copy(uint16_t *frame,int x,int y,int w,int h,const uint16_t *pixels,uint16_t mask){
 PortraitFrame::copy(frame,480,x,y,w,h,pixels,mask);
}
}
