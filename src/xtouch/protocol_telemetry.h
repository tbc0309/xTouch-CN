#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <string.h>
#include <cmath>
#include <cstdlib>

namespace XTouchTelemetry {
struct Bed {bool present=false,actualValid=false,targetValid=false;unsigned actual=0,target=0;};
inline Bed bed(JsonObjectConst report){
    Bed out;JsonObjectConst info=report["device"]["bed"]["info"];
    JsonVariantConst value=info.containsKey("temp")?info["temp"]:report.containsKey("bed_temp")?report["bed_temp"]:report["device"]["bed_temp"];
    out.present=info.containsKey("temp")||report.containsKey("bed_temp")||report["device"].containsKey("bed_temp");
    if(!value.is<int64_t>()&&!value.is<uint64_t>())return out;
    int64_t number=value.as<int64_t>();if(number<INT32_MIN||number>UINT32_MAX)return out;
    uint32_t bits=uint32_t(number);out.actual=bits&65535U;out.target=bits>>16;
    out.actualValid=out.actual<=160;out.targetValid=out.target<=160;return out;
}
struct Chamber {
    bool actualPresent=false,targetPresent=false,actualValid=false,targetValid=false;
    double actual=0,target=0;
};
inline Chamber chamber(JsonObjectConst report){
    Chamber out;
    auto number=[](JsonVariantConst v,double low,double high,double &value){
        if(v.isNull()||v.is<bool>())return false;
        if(v.is<const char*>()){const char *text=v.as<const char*>();char *end;value=strtod(text,&end);if(!*text||*end)return false;}
        else if(v.is<double>())value=v.as<double>();else return false;
        return std::isfinite(value)&&value>=low&&value<=high;
    };
    out.actualPresent=report.containsKey("chamber_temper");
    out.targetPresent=report.containsKey("ctt");
    if(out.actualPresent)out.actualValid=number(report["chamber_temper"],-20,200,out.actual);
    if(out.targetPresent)out.targetValid=number(report["ctt"],0,100,out.target);
    JsonObjectConst info=report["device"]["ctc"]["info"];
    if(info.containsKey("temp")){
        out.actualPresent=out.targetPresent=true;out.actualValid=out.targetValid=false;
        JsonVariantConst value=info["temp"];
        if(value.is<uint64_t>()&&value.as<uint64_t>()>UINT32_MAX)return out;
        if(value.is<int64_t>()||value.is<uint64_t>()){
            int64_t n=value.as<int64_t>();
            if(n>=INT32_MIN&&n<=UINT32_MAX){uint32_t bits=uint32_t(n);out.actual=bits&65535u;out.target=bits>>16;out.actualValid=out.actual<=200;out.targetValid=out.target<=100;}
        }
    }
    return out;
}
inline bool chamberLight(JsonObjectConst report,bool &on){
    for(JsonObjectConst light:report["lights_report"].as<JsonArrayConst>()){
        if(strcmp(light["node"]|"","chamber_light"))continue;
        const char *mode=light["mode"]|"";if(strcmp(mode,"on")&&strcmp(mode,"off"))return false;
        on=!strcmp(mode,"on");return true;
    }
    return false;
}
struct Fans {bool present=false;unsigned mask=0;uint8_t id[3]={},speed[3]={};};
inline Fans fans(JsonObjectConst report){
    Fans out;JsonVariantConst parts=report["device"]["airduct"]["parts"];out.present=parts.is<JsonArrayConst>();unsigned duplicates=0;
    for(JsonObjectConst part:parts.as<JsonArrayConst>()){
        if(!part["id"].is<unsigned>()||!part["state"].is<unsigned>()||!part["range"].is<uint32_t>()||!part["func"].is<unsigned>())continue;
        unsigned packed=part["id"],func=part["func"],state=part["state"].as<unsigned>()&255U;uint32_t range=part["range"];
        if((packed&15U)!=0||func<1||func>3||(range&65535U)!=0||(range>>16)!=100||state>100)continue;
        unsigned index=func-1,bit=1U<<index;if(out.mask&bit){duplicates|=bit;continue;}
        out.mask|=bit;out.id[index]=(packed>>4)&255U;out.speed[index]=(state*255U+50)/100;
    }
    out.mask&=~duplicates;return out;
}
}
