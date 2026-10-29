#ifndef XTOUCH_NATIVE_IDF_DIGEST_H
#define XTOUCH_NATIVE_IDF_DIGEST_H
#pragma once
#include "runtime.h"
#include <esp_rom_md5.h>
class MD5Builder {
 md5_context_t state{};uint8_t digest[16]{};
public:
 void begin(){esp_rom_md5_init(&state);}
 void add(const uint8_t *p,size_t n){esp_rom_md5_update(&state,p,n);}
 void add(const char *p){add((const uint8_t*)p,strlen(p));}
 void add(const String &s){add((const uint8_t*)s.data(),s.size());}
 template<class Source>bool addStream(Source &source,size_t size){uint8_t b[512];size_t done=0;while(done<size){size_t n=source.readBytes(b,std::min(sizeof(b),size-done));if(!n)return false;add(b,n);done+=n;}return true;}
 void calculate(){esp_rom_md5_final(digest,&state);}
 String toString()const{char b[33];for(unsigned i=0;i<16;++i)snprintf(b+i*2,3,"%02x",digest[i]);return b;}
 void getBytes(uint8_t *out)const{memcpy(out,digest,16);}
};

#endif
