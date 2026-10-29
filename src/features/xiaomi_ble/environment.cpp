#include "features/config.h"
#if XTOUCH_FEATURE_XIAOMI_BLE
#include "home_view.hpp"
#include "environment.h"
#include "display_preferences.h"
#include "xiaomi_ble.h"
#include "home_environment.h"
#include "xtouch/storage.h"
#include "xtouch/vault.h"
#include "xtouch/types.h"
#include <cstring>
#include "esp_log.h"
namespace {
constexpr const char *path=xtouch_environment_config_path;
constexpr uint32_t freshMs=HomeEnvironment::freshMs;
portMUX_TYPE configGuard=portMUX_INITIALIZER_UNLOCKED;
bool configured=false,keyReady=false,homeVisible=false,encrypted=false;
uint8_t displayMode=1,intervalSeconds=5;
int pendingDisplay=-1;
char selectedMac[18]={};
uint8_t mode(JsonVariantConst doc){const char *v=doc["display"]|"";return !strcmp(v,"off")?0:!strcmp(v,"sensor")?2:1;}
const char *modeName(uint8_t value){return value==0?"off":value==2?"sensor":"alternate";}
bool validKey(const char *key){if(!key||strlen(key)!=32)return false;for(const char *p=key;*p;++p)if(!isxdigit((unsigned char)*p))return false;return true;}
bool openKey(JsonVariantConst doc,const char *mac,String &key){if(!doc.containsKey("key")&&!doc.containsKey("keyEnc")){key="";return true;}if(doc.containsKey("key")){key=doc["key"]|"";return validKey(key.c_str());}return xtouch_vault_open(doc["keyEnc"]|"",String("environment:")+mac,key)&&validKey(key.c_str());}
void options(JsonVariantConst doc){portENTER_CRITICAL(&configGuard);displayMode=mode(doc);intervalSeconds=doc["interval"]|5;encrypted=doc.containsKey("keyEnc");pendingDisplay=-1;portEXIT_CRITICAL(&configGuard);}
bool canonical(const char *s,char *out){
 if(!s||strlen(s)!=17)return false;
 for(unsigned i=0;i<17;++i){if(i%3==2){if(s[i]!=':')return false;out[i]=':';}else{if(!isxdigit((unsigned char)s[i]))return false;out[i]=toupper((unsigned char)s[i]);}}
 out[17]=0;return true;
}
void apply(JsonVariantConst doc){
 char mac[18]={};bool present=canonical(doc["mac"]|"",mac);String key;
 bool opened=present&&openKey(doc,mac,key);
 xtouch_xiaomi_ble_unbind();bool ready=opened&&xtouch_xiaomi_ble_bind(mac,key.c_str())&&!key.isEmpty();
 key="";
 portENTER_CRITICAL(&configGuard);configured=present;keyReady=ready;memcpy(selectedMac,mac,sizeof(mac));portEXIT_CRITICAL(&configGuard);options(doc);
}
}
bool xtouch_environment_valid(JsonVariantConst doc){
 if(!doc.is<JsonObjectConst>())return false;
 if(doc.size()==0)return true;
 char mac[18];if(!canonical(doc["mac"]|"",mac))return false;
 bool plain=doc.containsKey("key"),sealed=doc.containsKey("keyEnc");if(plain&&sealed)return false;
 if(plain&&!validKey(doc["key"]|""))return false;
 if(sealed){const char *key=doc["keyEnc"]|"";if(strncmp(key,"v1:",3)||strlen(key)<40||strlen(key)>=512)return false;}
 if(doc.containsKey("display")){const char *v=doc["display"]|"";if(strcmp(v,"off")&&strcmp(v,"alternate")&&strcmp(v,"sensor"))return false;}
 if(doc.containsKey("interval")&&(!doc["interval"].is<unsigned>()||doc["interval"].as<unsigned>()<1||doc["interval"].as<unsigned>()>30))return false;
 return !doc.containsKey("home");
}
void xtouch_environment_begin(){auto doc=xtouch_storage_read(path,0);if(xtouch_environment_valid(doc.as<JsonVariantConst>()))apply(doc.as<JsonVariantConst>());}
extern "C" bool xtouch_environment_display_get(uint8_t *value){
 portENTER_CRITICAL(&configGuard);bool present=configured;if(value)*value=pendingDisplay>=0?pendingDisplay:displayMode;portEXIT_CRITICAL(&configGuard);return present;
}
extern "C" bool xtouch_environment_display_request(uint8_t value){
 portENTER_CRITICAL(&configGuard);bool accepted=configured&&value<=2;if(accepted)pendingDisplay=value;portEXIT_CRITICAL(&configGuard);return accepted;
}
extern "C" void xtouch_environment_display_poll(){
 portENTER_CRITICAL(&configGuard);int requested=pendingDisplay;pendingDisplay=-1;portEXIT_CRITICAL(&configGuard);
 if(requested<0)return;
 /* Preserve MAC, key/encryption and interval. Never perform storage writes in
  * an LVGL event, and never manufacture a binding from a display preference. */
 auto doc=xtouch_storage_read(path,0);
 if(!doc["mac"].is<const char*>())return;
 doc["display"]=modeName(requested);
 if(doc.overflowed()||!xtouch_environment_store(doc.as<JsonVariantConst>()))ESP_LOGE("environment","Display preference save failed");
}
bool xtouch_environment_store(JsonVariantConst doc){
 if(!xtouch_environment_valid(doc))return false;
 if(doc.size()==0){if(!xtouch_storage_delete(path))return false;apply(doc);return true;}
 DynamicJsonDocument saved(1024);saved["mac"]=doc["mac"];if(doc.containsKey("keyEnc"))saved["keyEnc"]=doc["keyEnc"];else if(doc.containsKey("key"))saved["key"]=doc["key"];saved["display"]=modeName(mode(doc));saved["interval"]=doc["interval"]|5;
 auto old=xtouch_storage_read(path,0);char previousMac[18];String previousKey,newKey;
 bool same=canonical(old["mac"]|"",previousMac)&&!strcmp(previousMac,saved["mac"]|"")&&openKey(old.as<JsonVariantConst>(),previousMac,previousKey)&&openKey(saved.as<JsonVariantConst>(),previousMac,newKey)&&previousKey==newKey;
 previousKey="";newKey="";
 if(saved.overflowed()||!xtouch_storage_write(path,saved))return false;
 if(!same)apply(saved.as<JsonVariantConst>());else options(saved.as<JsonVariantConst>());return true;
}
bool xtouch_environment_save(JsonVariantConst input){
 if(input["clear"]==true){StaticJsonDocument<32> empty;empty.to<JsonObject>();return xtouch_environment_store(empty.as<JsonVariantConst>());}
 char mac[18];const char *display=input["display"]|"";
 if(!canonical(input["mac"]|"",mac)||!input["encrypt"].is<bool>()||!input["interval"].is<unsigned>()||input["interval"].as<unsigned>()<1||input["interval"].as<unsigned>()>30||(strcmp(display,"off")&&strcmp(display,"alternate")&&strcmp(display,"sensor")))return false;
 String key=input["bindkey"]|"",existingSeal;
 if(key.isEmpty()){
  auto old=xtouch_storage_read(path,0);char previous[18];
  if(canonical(old["mac"]|"",previous)&&!strcmp(previous,mac)){
   if(!openKey(old.as<JsonVariantConst>(),mac,key))return false;
   existingSeal=old["keyEnc"]|"";
  } // A new MAC never inherits the previous sensor key.
 }
 if(!key.isEmpty()&&!validKey(key.c_str()))return false;
 StaticJsonDocument<1024> doc;doc["mac"]=mac;doc["display"]=display;doc["interval"]=input["interval"].as<unsigned>();
 if(input["encrypt"].as<bool>()){
  if(key.isEmpty())return false;
  String sealed=existingSeal.isEmpty()?xtouch_vault_seal(key,String("environment:")+mac):existingSeal;if(sealed.isEmpty())return false;doc["keyEnc"]=sealed;
 }else if(!key.isEmpty())doc["key"]=key;
 key="";return xtouch_environment_store(doc.as<JsonVariantConst>());
}
void xtouch_environment_status(JsonObject out){
 char mac[18];bool present,ready,visible,protect;uint8_t mode,seconds;
 portENTER_CRITICAL(&configGuard);memcpy(mac,selectedMac,sizeof(mac));present=configured;ready=keyReady;mode=displayMode;seconds=intervalSeconds;protect=encrypted;visible=homeVisible;portEXIT_CRITICAL(&configGuard);
 out["configured"]=present;out["mac"]=mac;out["keySaved"]=ready;out["encrypted"]=protect;out["display"]=modeName(mode);out["interval"]=seconds;out["hasAms"]=bambuStatus.ams_exist_bits!=0;out["homeVisible"]=visible;
 float t,rh;uint32_t age,ta,ha,seen;bool has=xtouch_xiaomi_ble_measurement(t,rh,age,&ta,&ha,&seen);xtouch_xiaomi_ble_model_status(out);out["available"]=present&&has;out["fresh"]=present&&has&&age<=freshMs;out["keyRequired"]=xtouch_xiaomi_ble_key_required();out["keyUnavailable"]=protect&&!ready;
 if(has){out["temperature"]=t;out["humidity"]=rh;out["ageMs"]=age;out["temperatureAgeMs"]=ta;out["humidityAgeMs"]=ha;out["lastSeenAgeMs"]=seen;}
}
extern "C" void ui_home_environment_bind(lv_obj_t *strip,lv_obj_t *clockGap){
 HomeEnvironment::bind(strip,clockGap,{
  [](){
   HomeEnvironment::Options options;
   portENTER_CRITICAL(&configGuard);options.configured=configured;options.mode=displayMode;options.interval=intervalSeconds;portEXIT_CRITICAL(&configGuard);
   options.hasAms=bambuStatus.ams_exist_bits!=0;return options;
  },
  [](float &temperature,float &humidity,uint32_t &age){return xtouch_xiaomi_ble_measurement(temperature,humidity,age);},
  [](bool visible){portENTER_CRITICAL(&configGuard);homeVisible=visible;portEXIT_CRITICAL(&configGuard);}
 });
}
#endif
