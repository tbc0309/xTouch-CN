#pragma once
#include <ArduinoJson.h>
#include <string.h>
#include <ctype.h>
#include "types.h"

namespace XTouchCamera {
// Packed cfg/stat values are hexadecimal strings; inspect bits without
// truncating larger reports to a machine integer.
inline bool hexBit(JsonVariantConst value,unsigned bit,bool &out) {
    if(!value.is<const char*>())return false;
    const char *s=value.as<const char*>();size_t n=strlen(s);
    if(n>132)return false;
    while(n && isspace((unsigned char)*s)){++s;--n;}
    while(n && isspace((unsigned char)s[n-1]))--n;
    if(n>=2 && s[0]=='0' && (s[1]=='x'||s[1]=='X')){s+=2;n-=2;}
    if(!n || n>128)return false;
    for(size_t i=0;i<n;++i)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')||(s[i]>='A'&&s[i]<='F')))return false;
    if(bit/4>=n){out=false;return true;}
    char c=s[n-1-bit/4];unsigned digit=c<='9'?c-'0':(c|32)-'a'+10;
    out=(digit>>(bit%4))&1;return true;
}
inline bool setting(JsonVariantConst value,bool &out) {
    const char *s=value.is<const char*>()?value.as<const char*>():nullptr;
    if(!s)return false;
    if(!strcmp(s,"enable")){out=true;return true;}
    if(!strcmp(s,"disable")){out=false;return true;}
    return false;
}
inline void reset(XTouchBambuStatus &s) {
    s.camera_recording=false;s.camera_recording_valid=false;
    s.camera_status_packed=false;s.camera_settings_packed=false;
    s.camera_recording_when_printing=false;s.camera_timelapse=false;s.has_ipcam=false;
}
inline bool update(JsonObjectConst p,XTouchBambuStatus &s) {
    bool changed=false,value;
    auto assign=[&](bool &target,bool next){changed|=target!=next;target=next;};
    JsonObjectConst ipcam=p["ipcam"].as<JsonObjectConst>();
    if(!s.camera_settings_packed && setting(ipcam["ipcam_record"],value))assign(s.camera_recording_when_printing,value);
    if(!s.camera_settings_packed && setting(ipcam["timelapse"],value))assign(s.camera_timelapse,value);
    const char *present=ipcam["ipcam_dev"]|"";
    if(!strcmp(present,"1")||!strcmp(present,"0"))assign(s.has_ipcam,*present=='1');
    if(hexBit(p["cfg"],3,value)){assign(s.camera_recording_when_printing,value);s.camera_settings_packed=true;}
    if(hexBit(p["cfg"],5,value))assign(s.camera_timelapse,value);
    bool known=hexBit(p["stat"],7,value);
    if(known)s.camera_status_packed=true;
    if(!known && !s.camera_status_packed && p["home_flag"].is<int64_t>()) {
        int64_t flag=p["home_flag"].as<int64_t>();
        if(flag>=INT32_MIN && flag<=UINT32_MAX){value=(uint32_t(flag)&0x20U)!=0;known=true;}
    }
    if(known){assign(s.camera_recording,value);assign(s.camera_recording_valid,true);}
    return changed;
}
}
