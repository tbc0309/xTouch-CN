#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "camera_frame.h"
namespace CameraProtocol {
enum class Stream { Unknown, JpegTls, Rtsps };
/* Explicit aliases: unknown/new printers must not be probed with the wrong protocol. */
inline Stream stream(const char *model){
 if(!model)return Stream::Unknown;
 const char *jpeg[]={"C11","C12","N1","N2S","P1P","P1S","A1","A1MINI","A1 mini"};
 const char *rtsp[]={"BL-P001","BL-P002","BLP001","BLP002","C13","N6","N7","O1D","O1C","O1C2","O1S","O1E","O2D","X1","X1C","X1 Carbon","X1E","X2D","P2S","H2C","H2D","H2S","H2DPRO","H2D Pro"};
 for(auto *name:jpeg)if(!strcmp(model,name))return Stream::JpegTls;
 for(auto *name:rtsp)if(!strcmp(model,name))return Stream::Rtsps;
 return Stream::Unknown;
}
constexpr size_t maxFrame=CameraFrame::jpegCapacity;
inline size_t frameLength(const uint8_t header[16]){uint32_t n=uint32_t(header[0])|(uint32_t(header[1])<<8)|(uint32_t(header[2])<<16)|(uint32_t(header[3])<<24);return n>=4&&n<=maxFrame?n:0;}
inline bool jpeg(const uint8_t *p,size_t n){return n>=4&&p[0]==0xff&&p[1]==0xd8&&p[n-2]==0xff&&p[n-1]==0xd9;}
inline bool auth(const char *code,uint8_t packet[80]){if(!code||strlen(code)!=8)return false;memset(packet,0,80);packet[0]=0x40;packet[4]=0x00;packet[5]=0x30;memcpy(packet+16,"bblp",4);memcpy(packet+48,code,8);return true;}
template<class Client,class Clock,class Pause,class Stop>
bool receive(Client &client,uint8_t *p,size_t n,Clock now,Pause pause,Stop stop,uint32_t totalMs=6000,uint32_t idleMs=3000){
 uint32_t started=now(),last=started;
 while(n&&!stop()){
  if(uint32_t(now()-started)>=totalMs||uint32_t(now()-last)>=idleMs)return false;
  int available=client.available();if(available>0){size_t take=n<size_t(available)?n:size_t(available);int got=client.read(p,take);if(got<=0||size_t(got)>take)return false;p+=got;n-=got;last=now();}
  else {if(!client.connected())return false;pause();}
 }
 return n==0&&!stop();
}
inline bool lan(const uint8_t ip[4]){return ip[0]==10||(ip[0]==172&&ip[1]>=16&&ip[1]<=31)||(ip[0]==192&&ip[1]==168);}
}
