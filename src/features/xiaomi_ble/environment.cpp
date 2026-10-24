#include "features/config.h"
#if XTOUCH_FEATURE_XIAOMI_BLE
#include "environment.h"
#include "xiaomi_ble.h"
#include "home_environment.h"
#include "ui/layout_shared.h"
#include "xtouch/storage.h"
#include "xtouch/vault.h"
#include "xtouch/types.h"
#include <cstring>
#include <new>
#ifdef lv_txt_get_size
#undef lv_txt_get_size
#endif
LV_FONT_DECLARE(ui_font_text_14);
namespace {
constexpr const char *path=xtouch_environment_config_path;
constexpr uint32_t freshMs=600000;
portMUX_TYPE configGuard=portMUX_INITIALIZER_UNLOCKED;
bool configured=false,keyReady=false,homeVisible=false,encrypted=false;
uint8_t displayMode=1,intervalSeconds=5;
char selectedMac[18]={};
uint8_t mode(JsonVariantConst doc){const char *v=doc["display"]|"";return !strcmp(v,"off")?0:!strcmp(v,"sensor")?2:1;}
const char *modeName(uint8_t value){return value==0?"off":value==2?"sensor":"alternate";}
bool validKey(const char *key){if(!key||strlen(key)!=32)return false;for(const char *p=key;*p;++p)if(!isxdigit((unsigned char)*p))return false;return true;}
bool openKey(JsonVariantConst doc,const char *mac,String &key){if(!doc.containsKey("key")&&!doc.containsKey("keyEnc")){key="";return true;}if(doc.containsKey("key")){key=doc["key"]|"";return validKey(key.c_str());}return xtouch_vault_open(doc["keyEnc"]|"",String("environment:")+mac,key)&&validKey(key.c_str());}
void options(JsonVariantConst doc){portENTER_CRITICAL(&configGuard);displayMode=mode(doc);intervalSeconds=doc["interval"]|5;encrypted=doc.containsKey("keyEnc");portEXIT_CRITICAL(&configGuard);}
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
struct Home {lv_obj_t *strip,*slot,*label,*clock;uint32_t born;bool showing=false;};
void update(lv_timer_t *timer){
 auto *h=static_cast<Home*>(timer->user_data);bool ready;uint8_t mode,seconds;
 portENTER_CRITICAL(&configGuard);ready=configured;mode=displayMode;seconds=intervalSeconds;portEXIT_CRITICAL(&configGuard);
 float t=0,rh=0;uint32_t age=0;bool available=ready&&mode&&xtouch_xiaomi_ble_measurement(t,rh,age);
 bool hasAms=bambuStatus.ams_exist_bits!=0;
 bool visible=available&&(mode==2||!hasAms||((lv_tick_get()-h->born)/(uint32_t(seconds)*1000))%2==1);
 if(visible){lv_obj_set_style_text_font(h->label,lv_obj_get_style_text_font(h->clock,0),0);char text[40];snprintf(text,sizeof(text),"%s%.1f\xC2\xB0" "C %.0f%%",age>freshMs?"~":"",double(t),double(rh));if(strcmp(lv_label_get_text(h->label),text))lv_label_set_text(h->label,text);lv_point_t size;lv_txt_get_size(&size,text,lv_obj_get_style_text_font(h->clock,0),lv_obj_get_style_text_letter_space(h->clock,0),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);lv_obj_set_width(h->slot,size.x);lv_obj_add_flag(h->strip,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(h->slot,LV_OBJ_FLAG_HIDDEN);}
 else{lv_obj_add_flag(h->slot,LV_OBJ_FLAG_HIDDEN);if(h->showing)lv_obj_clear_flag(h->strip,LV_OBJ_FLAG_HIDDEN);}
 ui_layout_status_text(h->label);
 h->showing=visible;portENTER_CRITICAL(&configGuard);homeVisible=visible;portEXIT_CRITICAL(&configGuard);
}
void removed(lv_event_t *e){auto *timer=static_cast<lv_timer_t*>(lv_event_get_user_data(e));delete static_cast<Home*>(timer->user_data);lv_timer_del(timer);}
}
bool xtouch_environment_valid(JsonVariantConst doc){
 if(!doc.is<JsonObjectConst>())return false;if(doc.size()==0)return true;
 char mac[18];if(!canonical(doc["mac"]|"",mac))return false;
 bool plain=doc.containsKey("key"),sealed=doc.containsKey("keyEnc");if(plain&&sealed)return false;
 if(plain&&!validKey(doc["key"]|""))return false;
 if(sealed){const char *key=doc["keyEnc"]|"";if(strncmp(key,"v1:",3)||strlen(key)<40||strlen(key)>=512)return false;}
 if(doc.containsKey("display")){const char *v=doc["display"]|"";if(strcmp(v,"off")&&strcmp(v,"alternate")&&strcmp(v,"sensor"))return false;}
 if(doc.containsKey("interval")&&(!doc["interval"].is<unsigned>()||doc["interval"].as<unsigned>()<1||doc["interval"].as<unsigned>()>30))return false;
 return !doc.containsKey("home");
}
void xtouch_environment_begin(){auto doc=xtouch_storage_read(path,0);if(xtouch_environment_valid(doc.as<JsonVariantConst>()))apply(doc.as<JsonVariantConst>());}
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
 auto *h=new(std::nothrow) Home;if(!h)return;
 h->strip=strip;h->clock=lv_obj_get_child(clockGap,0);h->born=lv_tick_get();
 h->slot=lv_obj_create(lv_obj_get_parent(strip));lv_obj_remove_style_all(h->slot);lv_obj_set_size(h->slot,0,lv_pct(100));lv_obj_clear_flag(h->slot,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(h->slot,LV_OBJ_FLAG_HIDDEN);lv_obj_set_flex_grow(h->slot,0);
 h->label=lv_label_create(h->slot);lv_label_set_text(h->label,"");lv_obj_add_flag(h->label,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_clear_flag(h->label,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_text_font(h->label,lv_obj_get_style_text_font(h->clock,0),0);lv_obj_set_style_text_letter_space(h->label,lv_obj_get_style_text_letter_space(h->clock,0),0);lv_obj_set_style_text_color(h->label,lv_color_hex(UI_COLOR_BLUE),0);lv_obj_align(h->label,LV_ALIGN_CENTER,0,0);
 auto *timer=lv_timer_create(update,1000,h);if(!timer){lv_obj_del(h->slot);delete h;return;}lv_obj_add_event_cb(strip,removed,LV_EVENT_DELETE,timer);update(timer);
}
#endif
