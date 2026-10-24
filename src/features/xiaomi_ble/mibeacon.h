#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
// Protocol implementation kept independent of the MicroPython reference code.
// AES-CCM authentication is mandatory before any encrypted reading is accepted.
namespace xiaomi_ble {
enum class Result { NotMiBeacon, Invalid, NeedsKey, AuthenticationFailed, Unsupported, Reading };
struct Reading { bool temperature=false,humidity=false,battery=false;float t=0,h=0;uint16_t product=0;uint8_t b=0,action=0,counter=0;bool encrypted=false;uint8_t identity[6]={}; };
using Decrypt=bool(*)(const uint8_t*,const uint8_t*,const uint8_t*,size_t,uint8_t*);
struct Model {uint16_t product;const char *code,*maker;bool encrypted;};
inline const Model *model(uint16_t product){
 static const Model models[]={
  {0x520b,"KS2BB","Linptech",true},{0x055b,"LYWSD03MMC","MiaoMiaoce",true},
  {0x2832,"MJWSD05MMC","MiaoMiaoce",true},{0x4c47,"MJWSD05MMC","MiaoMiaoce",true},
  {0x55b5,"MJWSD06MMC","MiaoMiaoce",true},{0x5bea,"MJWSD06MMC","MiaoMiaoce",true},
  {0x16e4,"LYWSD02MMC","MiaoMiaoce",true},{0x2542,"LYWSD02MMC","MiaoMiaoce",true},
  {0x1203,"XMWSDJ04MMC","MiaoMiaoce",true},{0x01aa,"LYWSDCGQ","Xiaomi",false},
  {0x045b,"LYWSD02","Xiaomi",false},{0x0347,"CGG1","Qingping",false},
  {0x0b48,"CGG1","Qingping",true},{0x066f,"CGDK2","Qingping",false},
  {0x4f59,"CGDK3","Qingping",true},{0x0576,"CGD1","Qingping",false},
  {0x06d3,"MHO-C303","MiaoMiaoce",false},{0x0387,"MHO-C401","MiaoMiaoce",false},
  {0x0c3c,"CGC1","Qingping",false},{0x5db1,"MBS17","Xiaomi",true},
  {0x78db,"ESM787","Yanmi",true},{0x5808,"CGG3","Qingping",true},
  {0x9ab9,"MJWSD07MMC","Xiaomi",true}
 };
 for(const auto &item:models)if(item.product==product)return &item;return nullptr;
}
inline const char *model_name(uint16_t product){auto m=model(product);return m?m->code:"";}
inline bool requires_encryption(uint16_t product){auto m=model(product);return !m||m->encrypted;}
inline uint16_t le16(const uint8_t*p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline Result payload(const uint8_t*p,size_t n,Reading &out){
 if(n<2)return Result::Invalid;
 uint16_t id=le16(p);
 if((id==0x4801||id==0x4808)&&(n==6||(n==7&&p[2]==4))){
  float value;memcpy(&value,p+(n==7?3:2),4);
  if(!std::isfinite(value))return Result::Invalid;
  if(id==0x4801){if(value < -50 || value > 125)return Result::Invalid;out.temperature=true;out.t=value;}
  else{if(value<0||value>100)return Result::Invalid;out.humidity=true;out.h=std::trunc(value);}
 }else if(id==0x4802 && n==4){
  // MiBeacon object header: ID (2), payload length (1), uint8 humidity.
  if(p[2]!=1||p[3]>100)return Result::Invalid;out.humidity=true;out.h=p[3];
 }
 else if(id==0x5003 && n==3){if(p[2]>100)return Result::Invalid;out.battery=true;out.b=p[2];}
 else if(n==3&&p[1]==0x56&&p[2]==0&&(p[0]==0x0c||p[0]==0x0d||p[0]==0x0e))out.action=p[0];
 else return Result::Unsupported;
 return Result::Reading;
}
inline Result xiaomi_payload(const uint8_t *p,size_t n,Reading &out){
 bool any=false;
 for(size_t pos=0;pos<n;){
  if(n-pos<3)return Result::Invalid;uint16_t id=le16(p+pos);size_t len=p[pos+2];pos+=3;if(len>n-pos)return Result::Invalid;const uint8_t *v=p+pos;
  if(id==0x1004&&len==2){float t=int16_t(le16(v))/10.f;if(t < -50 || t > 125)return Result::Invalid;out.temperature=true;out.t=t;any=true;}
  else if(id==0x1006&&len==2){float h=le16(v)/10.f;if(h>100)return Result::Invalid;out.humidity=true;out.h=std::trunc(h);any=true;}
  else if(id==0x100d&&len==4){float t=int16_t(le16(v))/10.f,h=le16(v+2)/10.f;if(t < -50 || t >125 || h>100)return Result::Invalid;out.temperature=out.humidity=true;out.t=t;out.h=std::trunc(h);any=true;}
  else if((id==0x4801||id==0x4808)&&len==4){float value;memcpy(&value,v,4);if(!std::isfinite(value)||(id==0x4801?(value < -50 || value >125):(value<0||value>100)))return Result::Invalid;if(id==0x4801){out.temperature=true;out.t=value;}else{out.humidity=true;out.h=std::trunc(value);}any=true;}
  else if((id==0x4802||id==0x4803)&&len==1){if(v[0]>100)return Result::Invalid;if(id==0x4802){out.humidity=true;out.h=v[0];}else{out.battery=true;out.b=v[0];}any=true;}
  else if(id==0x100a&&len==1){if(v[0]>100)return Result::Invalid;out.battery=true;out.b=v[0];any=true;}
  pos+=len;
 }
 return any?Result::Reading:Result::Unsupported;
}
inline Result frame(const uint8_t*p,size_t n,const uint8_t addressLE[6],const uint8_t*key,Decrypt decrypt,Reading&out){
 if(n<5)return Result::Invalid;
 uint16_t control=le16(p);uint16_t product=le16(p+2);if(!*model_name(product))return Result::NotMiBeacon;
 // MiBeacon v2/v3 public payloads and authenticated v4/v5 payloads.
 // Legacy encrypted v2/v3 packets have no authenticated CCM tag and are excluded.
 unsigned version=control>>12;
 if(version<2||version>5||!(control&0x40)||(control&0x80)||((control&8)&&version<4))return Result::Unsupported;
 out={};out.product=product;out.counter=p[4];out.encrypted=control&8;size_t pos=5;
 uint8_t mac[6];memcpy(mac,addressLE,6);
 if(control&0x10){if(n-pos<6)return Result::Invalid;memcpy(mac,p+pos,6);pos+=6;}
 memcpy(out.identity,mac,6);
 if(control&0x20){if(pos>=n)return Result::Invalid;uint8_t capability=p[pos++];if(capability&0x20){if(pos>=n)return Result::Invalid;++pos;}}
 if(!out.encrypted){Result result=xiaomi_payload(p+pos,n-pos,out);return product==21003&&result!=Result::Reading?payload(p+pos,n-pos,out):result;}
 if(n-pos<8||n-pos>64)return Result::Invalid;
 if(!key)return Result::NeedsKey;
 uint8_t nonce[12];memcpy(nonce,mac,6);memcpy(nonce+6,p+2,3);
 memcpy(nonce+9,p+n-7,3);uint8_t plain[64]={};
 if(!decrypt||!decrypt(key,nonce,p+pos,n-pos,plain))return Result::AuthenticationFailed;
 Result result=xiaomi_payload(plain,n-pos-7,out);if(product==21003&&result!=Result::Reading)result=payload(plain,n-pos-7,out);memset(plain,0,sizeof(plain));return result;
}
// Parse actual AD lengths; never search for a byte pattern inside another AD.
inline Result advertisement(const uint8_t*p,size_t n,const uint8_t addressLE[6],const uint8_t*key,Decrypt decrypt,Reading&out){
 for(size_t pos=0;pos<n;){size_t len=p[pos++];if(!len)break;if(len>n-pos)return Result::Invalid;
  if(len>=3&&p[pos]==0x16&&p[pos+1]==0x95&&p[pos+2]==0xfe)return frame(p+pos+3,len-3,addressLE,key,decrypt,out);
  pos+=len;
 }return Result::NotMiBeacon;
}
}
