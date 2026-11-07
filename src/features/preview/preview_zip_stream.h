#pragma once
#include "preview_archive.h"
#include <PNGdec.h>
#include <algorithm>
#include <cstdlib>

namespace XTouchPreviewArchive {
// Shared with PNGdec: this bundled zlib uses caller-owned state/window memory.
// No second inflater implementation or unbounded archive buffer is needed.
struct Buffer {
 uint8_t *p=nullptr;
 ~Buffer(){std::free(p);}
 Buffer()=default;Buffer(const Buffer&)=delete;
};

// Bounded association check for an extracted G-code and its source package.
// This checks a prefix, not a whole-file integrity hash; retain the file identity.
struct MemberProbe { const char *name=nullptr;size_t limit=65536; };
struct GcodeProbe {
 const uint8_t *expected=nullptr;size_t bytes=0;uint32_t fileSize=0;
 char md5[33]{};bool hasMd5=false,matched=false;
};
template<class ReadSome,class Cancel,class Allocate>
int extract(ReadSome raw,Cancel cancel,Allocate allocate,unsigned plate,
            uint8_t *&png,size_t &pngSize,size_t &transferred,GcodeProbe *probe=nullptr,MemberProbe *memberProbe=nullptr){
 png=nullptr;pngSize=0;transferred=0;
 if(probe){probe->hasMd5=false;probe->matched=false;probe->md5[0]=0;}
 if(!plate||plate>999)return 25;
 uint8_t input[512],replay[28];size_t cursor=0,length=0,replayAt=0,replaySize=0;
 auto fill=[&](){
  if(cursor<length)return true;
  if(cancel()||transferred>=scanLimit)return false;
  int n=raw(input,std::min(sizeof(input),scanLimit-transferred));
  if(n<=0||size_t(n)>std::min(sizeof(input),scanLimit-transferred))return false;
  cursor=0;length=size_t(n);transferred+=length;return true;
 };
 auto read=[&](uint8_t *dest,size_t n){
  // A known-size member beyond the transfer budget cannot lead to a cover
  // within that budget. Stop at its header instead of downloading its body.
  if(n>scanLimit-transferred+(length-cursor)+(replaySize-replayAt))return false;
  while(n){
   if(replayAt<replaySize){size_t take=std::min(n,replaySize-replayAt);if(dest){std::memcpy(dest,replay+replayAt,take);dest+=take;}replayAt+=take;n-=take;continue;}
   if(!fill())return false;size_t take=std::min(n,length-cursor);
   if(dest){std::memcpy(dest,input+cursor,take);dest+=take;}cursor+=take;n-=take;
  }return true;
 };
 uint32_t nextHeader=0;
 Buffer image;
 auto append=[&](const uint8_t *bytes,size_t n,size_t &used,size_t limit,uint32_t &crc,bool target){
  if(n>limit-used)return false;
  if(target)std::memcpy(image.p+used,bytes,n);
  crc=uint32_t(::crc32(crc,bytes,n));used+=n;return true;
 };
 for(unsigned member=0;member<4096&&!cancel();++member){
  uint8_t h[30];
  if(nextHeader){for(unsigned i=0;i<4;++i)h[i]=uint8_t(nextHeader>>(8*i));nextHeader=0;}
  else if(!read(h,4))return 13;
  if(u32(h)==0x02014b50||u32(h)==0x06054b50)return 25;
  if(u32(h)!=0x04034b50)return 29;
  if(!read(h+4,26))return 13;
  unsigned flags=u16(h+6),method=u16(h+8),names=u16(h+26),extra=u16(h+28);
  if(flags&65||names>1024||extra>4096)return 29;
  char name[1025];if(!read(reinterpret_cast<uint8_t*>(name),names))return 13;name[names]=0;
  if(std::strlen(name)!=names)return 29;
  uint64_t compressed=u32(h+18),expanded=u32(h+22);uint32_t expectedCRC=u32(h+14);
  bool zip64=compressed==0xffffffffU||expanded==0xffffffffU;
  while(extra){
   if(extra<4)return 29;uint8_t field[4];if(!read(field,4))return 13;extra-=4;
   unsigned count=u16(field+2);if(count>extra)return 29;
   if(u16(field)==1){
    for(uint64_t *value:{&expanded,&compressed})if(*value==0xffffffffU){
     if(count<8)return 29;uint8_t v[8];if(!read(v,8))return 13;
     *value=uint64_t(u32(v))|uint64_t(u32(v+4))<<32;count-=8;extra-=8;
    }
   }
   if(!read(nullptr,count))return 13;extra-=count;
  }
  if(compressed==0xffffffffU||expanded==0xffffffffU)return 29;
  char full[64],small[64];std::snprintf(full,sizeof(full),"Metadata/plate_%u.png",plate);
  std::snprintf(small,sizeof(small),"Metadata/plate_%u_small.png",plate);
  const bool smallName=!std::strcmp(name,small);
  char gcodeName[64],md5Name[72];
  std::snprintf(gcodeName,sizeof(gcodeName),"Metadata/plate_%u.gcode",plate);
  std::snprintf(md5Name,sizeof(md5Name),"Metadata/plate_%u.gcode.md5",plate);
  const bool gcodeTarget=probe&&!std::strcmp(name,gcodeName),md5Target=probe&&!std::strcmp(name,md5Name);
  const bool memberTarget=memberProbe && memberProbe->name && !std::strcmp(name,memberProbe->name);
  const bool target=memberProbe?memberTarget:probe?(gcodeTarget||md5Target):(!std::strcmp(name,full)||smallName),descriptor=flags&8;
  if(gcodeTarget && (!probe->hasMd5 || !probe->expected || !probe->bytes || probe->bytes>1024 ||
       (expanded && expanded!=probe->fileSize)))return 25;
  if(gcodeTarget && method==0){
   Buffer prefix;prefix.p=(uint8_t*)allocate(probe->bytes);if(!prefix.p)return 12;
   if(!read(prefix.p,probe->bytes))return 13;
   probe->matched=!std::memcmp(prefix.p,probe->expected,probe->bytes);
   return probe->matched?0:25;
  }
  if(!target&&!descriptor){if(compressed>scanLimit||!read(nullptr,size_t(compressed)))return 13;continue;}
  if(method!=0&&method!=8)return 29;
  // Stored, non-image streaming members have no self-delimiting codec.
  // Locate their descriptor by verifying the entire preceding CRC and length,
  // then require the following ZIP header. Keep only a small look-ahead window.
  if(!target&&descriptor&&method==0){
   uint8_t pending[28];size_t held=0,used=0;uint32_t runningCRC=0;bool found=false;
   const size_t base=zip64?20:12;
   while(!cancel()){
    while(held<base){if(!read(pending+held,1))return 13;++held;}
    size_t offset=u32(pending)==0x08074b50?4:0;
    while(held<base+offset){if(!read(pending+held,1))return 13;++held;}
    auto sizeAt=[&](const uint8_t *v)->uint64_t{return zip64?uint64_t(u32(v))|(uint64_t(u32(v+4))<<32):u32(v);};
    const uint8_t *d=pending+offset;
    if(u32(d)==runningCRC&&sizeAt(d+4)==used&&sizeAt(d+4+(zip64?8:4))==used){
     while(held<base+offset+4){if(!read(pending+held,1))return 13;++held;}
     uint32_t signature=u32(pending+base+offset);
     if(signature==0x04034b50||signature==0x02014b50||signature==0x06054b50){nextHeader=signature;replayAt=0;replaySize=held-(base+offset+4);std::memcpy(replay,pending+base+offset+4,replaySize);found=true;break;}
    }
    runningCRC=uint32_t(::crc32(runningCRC,pending,1));++used;
    std::memmove(pending,pending+1,--held);
   }
   if(!found)return 13;
   continue;
  }

  const size_t outputLimit=memberTarget?std::min<size_t>(memberProbe->limit,65536):gcodeTarget?probe->bytes:md5Target?32:target?imageLimit:8*1024*1024;
  if(target){
   if(!probe&&!memberProbe&&!descriptor&&(!compressed||!expanded||compressed>imageLimit||expanded>imageLimit))return 11;
   if(md5Target && !descriptor && expanded!=32)return 25;
   if(memberTarget && ((!descriptor && (!expanded || expanded>outputLimit)) || (method==0 && descriptor)))return 11;
   image.p=(uint8_t*)allocate((probe||memberProbe)?outputLimit:descriptor?imageLimit:size_t(expanded));if(!image.p)return 12;
  }
  size_t produced=0,consumed=0;uint32_t crc=0;
  if(method==8){
   Buffer stateMemory;stateMemory.p=(uint8_t*)allocate(sizeof(inflate_state)+32768);if(!stateMemory.p)return 12;
   std::memset(stateMemory.p,0,sizeof(inflate_state)+32768);
   auto *state=(inflate_state*)stateMemory.p;state->window=stateMemory.p+sizeof(inflate_state);
   z_stream stream={};stream.state=(internal_state*)state;
   if(inflateInit2(&stream,-15)!=Z_OK)return 19;
   uint8_t output[1024];bool done=false;
   while(!cancel()){
    if(!fill())return 13;
    size_t available=length-cursor;
    if(!descriptor){if(consumed>=compressed)return 19;available=std::min(available,size_t(compressed-consumed));}
    stream.next_in=input+cursor;stream.avail_in=available;stream.next_out=output;stream.avail_out=sizeof(output);
    int result=::inflate(&stream,Z_NO_FLUSH,0);size_t took=available-stream.avail_in,n=sizeof(output)-stream.avail_out;
    cursor+=took;consumed+=took;
    if(gcodeTarget) {
     size_t take=std::min(n,probe->bytes-produced);std::memcpy(image.p+produced,output,take);produced+=take;
     if(produced==probe->bytes){probe->matched=!std::memcmp(image.p,probe->expected,probe->bytes);return probe->matched?0:25;}
    } else if(!append(output,n,produced,outputLimit,crc,target))return 11;
    if(target&&consumed>imageLimit)return 11;
    if(result==Z_STREAM_END){done=true;break;}
    if(result!=Z_OK||(!took&&!n))return 19;
   }
   // inflateEnd would free the preallocated window/state separately. Both are
   // owned by stateMemory, matching the bundled PNG decoder's allocation model.
   if(!done)return 13;
  }else if(descriptor&&md5Target){
   if(!read(image.p,32))return 13;produced=consumed=32;crc=uint32_t(::crc32(0,image.p,32));
  }else if(descriptor){
   uint8_t signature[8];if(!read(signature,8))return 13;
   const uint8_t pngSignature[]={137,80,78,71,13,10,26,10};
   if(std::memcmp(signature,pngSignature,8))return 29;
   if(!append(signature,8,produced,outputLimit,crc,target))return 11;
   bool ended=false;uint8_t scratch[1024];
   for(unsigned chunk=0;chunk<4096;++chunk){
    uint8_t chunkHeader[8];if(!read(chunkHeader,8))return 13;
    uint32_t n=uint32_t(chunkHeader[0])<<24|uint32_t(chunkHeader[1])<<16|uint32_t(chunkHeader[2])<<8|chunkHeader[3];
    const bool end=!std::memcmp(chunkHeader+4,"IEND",4);
    if(end&&n)return 19;
    if(!append(chunkHeader,8,produced,outputLimit,crc,target)||n>outputLimit-produced||4>outputLimit-produced-n)return 11;
    size_t left=size_t(n)+4;
    while(left){size_t take=std::min(left,sizeof(scratch));if(!read(scratch,take))return 13;
     if(!append(scratch,take,produced,outputLimit,crc,target))return 11;left-=take;
    }
    if(end){ended=true;break;}
   }
   if(!ended)return 19;consumed=produced;
  }else{
   uint8_t scratch[1024];size_t left=size_t(compressed);
   while(left){size_t take=std::min(left,sizeof(scratch));if(!read(scratch,take))return 13;
    if(!append(scratch,take,produced,outputLimit,crc,target))return 11;left-=take;
   }consumed=size_t(compressed);
  }
  if(descriptor){
   uint8_t d[24];if(!read(d,4))return 13;
   if(u32(d)==0x08074b50){if(!read(d,4))return 13;}
   const size_t sizeBytes=zip64?16:8;if(!read(d+4,sizeBytes))return 13;
   expectedCRC=u32(d);
   compressed=zip64?(uint64_t(u32(d+4))|uint64_t(u32(d+8))<<32):u32(d+4);
   expanded=zip64?(uint64_t(u32(d+12))|uint64_t(u32(d+16))<<32):u32(d+8);
  }
  if(consumed!=compressed||produced!=expanded||crc!=expectedCRC)return 19;
  if(md5Target) {
   if(produced!=32)return 25;
   for(unsigned i=0;i<32;++i){uint8_t c=image.p[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return 25;probe->md5[i]=c>='a'&&c<='f'?char(c-'a'+'A'):char(c);}
   probe->md5[32]=0;probe->hasMd5=true;std::free(image.p);image.p=nullptr;continue;
  }
  if(gcodeTarget)return 25;
  if(memberTarget){png=image.p;image.p=nullptr;pngSize=produced;return 0;}
  if(target){
   if(produced<24||std::memcmp(image.p,"\x89PNG\r\n\x1a\n",8)||std::memcmp(image.p+12,"IHDR",4))return 19;
   auto bigEndian=[](const uint8_t *v){return uint32_t(v[0])<<24|uint32_t(v[1])<<16|uint32_t(v[2])<<8|v[3];};
   unsigned width=bigEndian(image.p+16),height=bigEndian(image.p+20);
   if(!width||!height)return 20;
   // Use the first verified image for this plate; display decoding downsamples it.
   if(width>512||height>512)return 20;
   png=image.p;image.p=nullptr;pngSize=produced;return 0;
  }
 }
 return cancel()?13:11;
}
}
