#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "camera_frame.h"

// RFC 6184, packetization-mode 0/1. No network or decoder allocations here.
namespace CameraRtp {
struct Sps { unsigned width=0,height=0,refs=0,visibleWidth=0,visibleHeight=0; };
struct Bits {
 const uint8_t *p;size_t size,at=0;bool ok=true;
 unsigned get(unsigned n){unsigned v=0;while(n--){if(at>=size*8){ok=false;return 0;}v=(v<<1)|((p[at/8]>>(7-at%8))&1);++at;}return v;}
 unsigned ue(){unsigned zeros=0;while(ok&&!get(1)){if(++zeros>24){ok=false;return 0;}}return ((1u<<zeros)-1)+get(zeros);}
};
// Parse the allocation-relevant fields before the third-party decoder sees SPS.
inline bool sps(const uint8_t *p,size_t n,Sps &out){
 if(n<5||n>256||(p[0]&31)!=7||p[1]!=66)return false;
 uint8_t rbsp[256];size_t used=0;unsigned zeros=0;
 for(size_t i=1;i<n;++i){if(zeros>=2&&p[i]==3){zeros=0;continue;}rbsp[used++]=p[i];zeros=p[i]==0?zeros+1:0;}
 Bits b{rbsp,used};b.get(24);if(b.ue()>31||b.ue()>12)return false;unsigned poc=b.ue();
 if(poc==0){if(b.ue()>12)return false;}else if(poc==1){b.get(1);b.ue();b.ue();unsigned cycle=b.ue();if(cycle>255)return false;while(cycle--)b.ue();}else if(poc>2)return false;
 out.refs=b.ue();b.get(1);unsigned mw=b.ue(),mh=b.ue();bool progressive=b.get(1);if(!progressive)return false;
 b.get(1);unsigned left=0,right=0,top=0,bottom=0;
 if(b.get(1)){left=b.ue();right=b.ue();top=b.ue();bottom=b.ue();}
 if(!b.ok||mw>=256||mh>=256||out.refs>4)return false;
 unsigned w=(mw+1)*16,h=(mh+1)*16;
 if(uint64_t(left+uint64_t(right))*2>=w||uint64_t(top+uint64_t(bottom))*2>=h)return false;
 // Decoder returns coded-size I420; disallow non-zero left/top cropping.
 if(left||top)return false;
 out.width=w;out.height=h;out.visibleWidth=w-right*2;out.visibleHeight=h-bottom*2;
 return w<=1920&&h<=1088&&out.visibleWidth<=1920&&out.visibleHeight<=1080;
}
inline size_t decoderBytes(const Sps &s){return size_t(s.width)*s.height*(s.refs+3)*3/2+1024*1024;}
inline bool pps(const uint8_t *p,size_t n){
 if(n<2||n>256||(p[0]&31)!=8)return false;
 uint8_t rbsp[256];size_t used=0;unsigned zeros=0;
 for(size_t i=1;i<n;++i){if(zeros>=2&&p[i]==3){zeros=0;continue;}rbsp[used++]=p[i];zeros=p[i]==0?zeros+1:0;}
 Bits b{rbsp,used};if(b.ue()>255||b.ue()>31||b.get(1))return false;b.get(1);
 // Flexible macroblock ordering and CABAC are outside constrained baseline.
 if(b.ue()!=0||b.ue()>3||b.ue()>3)return false;
 return b.ok;
}
struct Assembler {
 uint8_t *data;size_t capacity,size=0;uint16_t sequence=0;uint32_t timestamp=0;bool seen=false,fragment=false;
 void reset(){size=0;fragment=false;}
 bool append(const uint8_t *p,size_t n){if(n>capacity-size){reset();return false;}memcpy(data+size,p,n);size+=n;return true;}
 template<class Emit> bool packet(const uint8_t *p,size_t n,unsigned payload,Emit emit){
  if(n<12||(p[0]>>6)!=2)return false;
  size_t head=12+4*(p[0]&15);if(head>n)return false;
  if(p[0]&16){if(head+4>n)return false;head+=4+4*((unsigned(p[head+2])<<8)|p[head+3]);if(head>n)return false;}
  if(p[0]&32){unsigned pad=p[n-1];if(!pad||pad>n-head)return false;n-=pad;}
  if((p[1]&127)!=payload||head>=n)return false;
  uint16_t seq=(unsigned(p[2])<<8)|p[3];uint32_t ts=(uint32_t(p[4])<<24)|(uint32_t(p[5])<<16)|(uint32_t(p[6])<<8)|p[7];
  if(seen&&(uint16_t(sequence+1)!=seq||ts!=timestamp))reset();
  seen=true;sequence=seq;timestamp=ts;
  p+=head;n-=head;unsigned type=p[0]&31;
  if(type>=1&&type<=23){reset();return emit(p,n);}
  if(type==24){reset();size_t i=1;while(i<n){if(i+2>n)return false;size_t len=(unsigned(p[i])<<8)|p[i+1];i+=2;if(!len||len>n-i||(p[i]&31)<1||(p[i]&31)>23||!emit(p+i,len))return false;i+=len;}return true;}
  if(type!=28||n<3||(p[1]&32)||(p[1]&31)==0||(p[1]&31)>23)return false;
  bool start=p[1]&128,end=p[1]&64;if(start&&end)return false;
  if(start){reset();fragment=true;uint8_t nal=(p[0]&224)|(p[1]&31);if(!append(&nal,1))return false;}
  else if(!fragment)return false;
  if(!append(p+2,n-2))return false;
  if(end){bool ok=emit(data,size);reset();return ok;}return true;
 }
};
template<class Stop,class Yield> bool i420(const uint8_t *src,unsigned w,unsigned h,uint16_t *out,unsigned cw,unsigned ch,uint16_t *columns,size_t columnCapacity,Stop stop,Yield yield,unsigned visibleWidth=0,unsigned visibleHeight=0){
 if(!src||!out||!w||!h||w>1920||h>1088||(w&1)||(h&1)||!cw||!ch)return false;
 if(!visibleWidth)visibleWidth=w;
 if(!visibleHeight)visibleHeight=h;
 if(visibleWidth>w||visibleHeight>h)return false;
 auto f=CameraFrame::fit(visibleWidth,visibleHeight,cw,ch);const uint8_t *u=src+w*h,*v=u+w*h/4;
 // Reuse the caller-owned session workspace; no per-frame allocation or stack map.
 // Compute divisions once per column/row rather than twice per output pixel.
 if(!columns||columnCapacity<unsigned(f.width))return false;
 for(unsigned x=0;x<unsigned(f.width);++x)columns[x]=x*visibleWidth/f.width;
 std::memset(out,0,size_t(cw)*ch*sizeof(uint16_t));
 for(unsigned y=0;y<unsigned(f.height);++y){
  if(stop())return false;if(!(y%16))yield();
  unsigned sy=y*visibleHeight/f.height,yyrow=sy*w,uvrow=(sy/2)*(w/2);
  uint16_t *row=out+(y+f.y)*cw+f.x;
  for(unsigned x=0;x<unsigned(f.width);++x){
   unsigned sx=columns[x];int yy=int(src[yyrow+sx])-16,uu=int(u[uvrow+sx/2])-128,vv=int(v[uvrow+sx/2])-128;
   auto clip=[](int a){return a<0?0:a>255?255:a;};int r=clip((298*yy+409*vv+128)>>8),g=clip((298*yy-100*uu-208*vv+128)>>8),b=clip((298*yy+516*uu+128)>>8);row[x]=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
  }
 }return true;
}
}
