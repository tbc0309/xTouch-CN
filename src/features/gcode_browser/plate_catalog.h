#pragma once
#include "policy.h"
#include <vector>
#include <algorithm>
namespace xtouch_files {
inline bool sameFile(const Entry &a,const Entry &b){return a.path==b.path && a.size==b.size && a.stamp==b.stamp;}
inline bool samePlate(const Entry &a,unsigned plate,const Entry &b,unsigned linkedPlate){return plate && plate==linkedPlate && sameFile(a,b);}
// Parse only a complete, bounded ZIP tail. Unsupported ZIP64/multi-disk layouts
// remain unknown; a PNG or a filename alone never proves a printable plate.
inline bool archivePlates(const uint8_t *tail,size_t bytes,size_t offset,size_t fileSize,std::vector<unsigned> &plates){
 plates.clear();if(bytes<22 || offset+bytes!=fileSize)return false;
 auto u16=[](const uint8_t *p){return unsigned(p[0])|unsigned(p[1])<<8;};
 auto u32=[](const uint8_t *p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;};
 size_t end=bytes-22;
 for(;;){if(u32(tail+end)==0x06054b50 && end+22+u16(tail+end+20)==bytes)break;if(!end)return false;--end;}
 const uint8_t *e=tail+end;
 unsigned count=u16(e+10);size_t size=u32(e+12),start=u32(e+16);
 if(u16(e+4)||u16(e+6)||u16(e+8)!=count||count==65535||start==0xffffffffu||size==0xffffffffu||start<offset||start>fileSize||size>fileSize-start||start+size!=offset+end)return false;
 size_t at=start-offset,limit=at+size;
 for(unsigned n=0;n<count;++n){
  if(at>limit || limit-at<46 || u32(tail+at)!=0x02014b50)return false;
  const uint8_t *h=tail+at;if(u32(h+42)>=start)return false;size_t names=u16(h+28),extra=u16(h+30),comment=u16(h+32),span=46+names+extra+comment;
  if(span>limit-at || names>1024)return false;
  std::string name((const char*)h+46,names);
  const std::string prefix="Metadata/plate_",suffix=".gcode";
  if(name.rfind(prefix,0)==0 && name.size()>prefix.size()+suffix.size() && name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0){
   unsigned plate=0;auto digits=name.substr(prefix.size(),name.size()-prefix.size()-suffix.size());
   if(digits.size()>3)return false;
   for(char c:digits){if(c<'0'||c>'9')return false;plate=plate*10+c-'0';}
   if(!plate || plate>999 || (u16(h+8)&65) || (u16(h+10)!=0 && u16(h+10)!=8) || !u32(h+24) || u32(h+24)==0xffffffffu)return false;
   if(std::find(plates.begin(),plates.end(),plate)!=plates.end())return false;
   plates.push_back(plate);
  }
  at+=span;
 }
 if(at!=limit)return false;
 std::sort(plates.begin(),plates.end());return true;
}
}
