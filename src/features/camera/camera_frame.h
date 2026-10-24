#pragma once
#include <stdint.h>
#include <stddef.h>
namespace CameraFrame {
constexpr size_t jpegCapacity=1024u*1024u;
constexpr uint32_t decodeBudgetMs=1500;
constexpr size_t psramReserve=512u*1024u;
struct Fit { int width,height,x,y; };
inline Fit fit(int width,int height,int canvasWidth,int canvasHeight){
 if(width<=0||height<=0||canvasWidth<=0||canvasHeight<=0)return {0,0,0,0};
 int w=canvasWidth,h=int(int64_t(height)*w/width);
 if(h>canvasHeight){h=canvasHeight;w=int(int64_t(width)*h/height);}
 if(w<1)w=1;
 if(h<1)h=1;
 return {w,h,(canvasWidth-w)/2,(canvasHeight-h)/2};
}
inline bool sourceAllowed(int w,int h){return w>0&&h>0&&w<=4096&&h<=4096&&int64_t(w)*h<=1920*1080;}
inline size_t requiredBytes(int w,int h){return size_t(w)*h*2*3+jpegCapacity;}
constexpr size_t jpegInternalBlock=20u*1024u;
constexpr size_t rtspInternalBlock=20u*1024u;
inline bool memoryAllowed(int w,int h,size_t freePsram,size_t largestPsram,size_t freeInternal,size_t largestInternal,bool rtsps=true){
 if(w<1||h<1||w>1024||h>600)return false;
 size_t frame=size_t(w)*h*2,largest=frame>jpegCapacity?frame:jpegCapacity;
 // Pixels, compressed input, decoder data and TLS buffers live in PSRAM.
 // Internal RAM retains the 12 KiB worker stack plus Wi-Fi/UI/DMA headroom;
 // no single 32 KiB allocation is needed. Keep the 80 KB startup reserve.
 return freePsram>=requiredBytes(w,h)+psramReserve&&largestPsram>=largest&&freeInternal>=80000&&largestInternal>=(rtsps?rtspInternalBlock:jpegInternalBlock);
}
/* Decoder pixels occupy the upper-left of a canvas-stride buffer. Enlarging
 * backwards keeps unread source pixels intact, using no fourth frame buffer. */
template<class Stop,class Yield>
bool present(uint16_t *pixels,int sw,int sh,int cw,int ch,Fit out,Stop stop,Yield yield){
 if(!pixels||sw<1||sh<1||sw>out.width||sh>out.height||out.width>cw||out.height>ch)return false;
 for(int y=ch-1;y>=0;--y){
  if(stop())return false;
  for(int x=cw-1;x>=0;--x){
   if(x<out.x||x>=out.x+out.width||y<out.y||y>=out.y+out.height)pixels[y*cw+x]=0;
   else pixels[y*cw+x]=pixels[((y-out.y)*sh/out.height)*cw+(x-out.x)*sw/out.width];
  }
  if((y&15)==0)yield();
 }
 return !stop();
}
}
