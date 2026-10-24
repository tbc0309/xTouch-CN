#include "features/config.h"
#if XTOUCH_FEATURE_XIAOMI_BLE
#include "xiaomi_ble.h"
#include "environment.h"
#include "mibeacon.h"
#include <Arduino.h>
#if CONFIG_IDF_TARGET_ESP32S3
#include <esp32-hal-alloc-ble-mem.h>
#elif CONFIG_IDF_TARGET_ESP32P4
#include <esp_hosted.h>
#include "hosted_bt_config.h"
#endif
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/ble_gap.h>
#include <host/util/util.h>
#include <psa/crypto.h>
#ifndef XTOUCH_P4_C6_PREINITIALIZED_BT
#define XTOUCH_P4_C6_PREINITIALIZED_BT 0
#endif

namespace {
portMUX_TYPE guard=portMUX_INITIALIZER_UNLOCKED;
struct Device {uint8_t mac[6]={};uint32_t packets=0,lastMs=0,encrypted=0,tAt=0,hAt=0;int rssi=0;bool used=false,authenticated=false;xiaomi_ble::Reading reading={};};
Device devices[8];uint32_t packets=0,invalid=0,other=0,overflow=0;int scanError=0;bool scanning=false,selftest=false;
uint8_t bindingMac[6]={},bindingKey[16]={};bool bound=false,keyPresent=false;uint32_t authFailures=0,bindingRevision=0;
// Bindkey is deliberately not guessed, bundled in firmware or written to logs.
bool decrypt(const uint8_t *key,const uint8_t *nonce,const uint8_t *encrypted,size_t n,uint8_t *plain){
 const psa_algorithm_t algorithm=PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM,4);
 psa_key_attributes_t attr=PSA_KEY_ATTRIBUTES_INIT;mbedtls_svc_key_id_t id=0;
 psa_set_key_type(&attr,PSA_KEY_TYPE_AES);psa_set_key_bits(&attr,128);psa_set_key_algorithm(&attr,algorithm);psa_set_key_usage_flags(&attr,PSA_KEY_USAGE_DECRYPT);
 int rc=psa_crypto_init();if(!rc)rc=psa_import_key(&attr,key,16,&id);psa_reset_key_attributes(&attr);
 uint8_t combined[64];size_t length=0,ciphertext=n-7;memcpy(combined,encrypted,ciphertext);memcpy(combined+ciphertext,encrypted+n-4,4);const uint8_t aad=0x11;
 if(!rc)rc=psa_aead_decrypt(id,algorithm,nonce,12,&aad,1,combined,ciphertext+4,plain,64,&length);
 if(id)psa_destroy_key(id);
 memset(combined,0,sizeof(combined));return rc==0&&length==ciphertext;
}
bool test(){
 const uint8_t key[16]={0,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
 // Synthetic reference fixture, never a user's address/key.
 uint8_t frame[]={0x58,0x59,0x0b,0x52,1,6,5,4,3,2,1,0x91,0x3f,0x3f,1,0,0,0x1c,0x85,0x8a,0xb2};
 const uint8_t mac[6]={6,5,4,3,2,1};xiaomi_ble::Reading r;
 if(xiaomi_ble::frame(frame,sizeof(frame),mac,key,decrypt,r)!=xiaomi_ble::Result::Reading||r.action!=0x0c)return false;
 frame[sizeof(frame)-1]^=1;
 return xiaomi_ble::frame(frame,sizeof(frame),mac,key,decrypt,r)==xiaomi_ble::Result::AuthenticationFailed;
}
int gap(ble_gap_event *event,void*){
 if(event->type==BLE_GAP_EVENT_DISC){
  auto &d=event->disc;xiaomi_ble::Reading r;uint8_t key[16]={};bool useKey=false;uint32_t revision=0;
  auto result=xiaomi_ble::advertisement(d.data,d.length_data,d.addr.val,nullptr,decrypt,r);
  portENTER_CRITICAL(&guard);bool selected=bound&&!memcmp(bindingMac,r.identity,6);portEXIT_CRITICAL(&guard);
  if(selected&&result==xiaomi_ble::Result::Reading&&!r.encrypted&&xiaomi_ble::requires_encryption(r.product))result=xiaomi_ble::Result::AuthenticationFailed;
  if(result==xiaomi_ble::Result::NeedsKey){
   portENTER_CRITICAL(&guard);useKey=bound&&keyPresent&&!memcmp(bindingMac,r.identity,6);if(useKey){memcpy(key,bindingKey,16);revision=bindingRevision;}portEXIT_CRITICAL(&guard);
   if(useKey)result=xiaomi_ble::advertisement(d.data,d.length_data,d.addr.val,key,decrypt,r);
  }
  memset(key,0,sizeof(key));
  portENTER_CRITICAL(&guard);++packets;
  if(useKey&&revision!=bindingRevision){++other;portEXIT_CRITICAL(&guard);return 0;}
  if(result==xiaomi_ble::Result::Invalid)++invalid;
  else if(result==xiaomi_ble::Result::AuthenticationFailed)++authFailures;
  else if(result==xiaomi_ble::Result::NeedsKey||result==xiaomi_ble::Result::Reading){
   int index=-1;for(int i=0;i<8;++i)if(devices[i].used&&!memcmp(devices[i].mac,r.identity,6)){index=i;break;}
   if(index<0)for(int i=0;i<8;++i)if(!devices[i].used){index=i;devices[i].used=true;memcpy(devices[i].mac,r.identity,6);break;}
   if(index<0&&bound&&!memcmp(bindingMac,r.identity,6)){index=0;devices[0]={};devices[0].used=true;memcpy(devices[0].mac,r.identity,6);}
   if(index>=0){auto &v=devices[index];if(v.reading.product!=r.product){v.reading={};v.authenticated=false;v.tAt=v.hAt=0;v.encrypted=0;}v.reading.product=r.product;v.reading.encrypted=r.encrypted;++v.packets;v.lastMs=millis();v.rssi=d.rssi;if(r.encrypted)++v.encrypted;if(result==xiaomi_ble::Result::Reading){v.authenticated=r.encrypted;v.reading.encrypted=r.encrypted;v.reading.product=r.product;if(r.temperature){v.reading.temperature=true;v.reading.t=r.t;v.tAt=millis();}if(r.humidity){v.reading.humidity=true;v.reading.h=r.h;v.hAt=millis();}if(r.battery){v.reading.battery=true;v.reading.b=r.b;}v.reading.action=r.action;}}
   else ++overflow;
  }else ++other;
  portEXIT_CRITICAL(&guard);
 }return 0;
}
void onSync(){
 uint8_t type=0;int e=ble_hs_util_ensure_addr(0);if(!e)e=ble_hs_id_infer_auto(0,&type);
 if(!e){ble_gap_disc_params p={};p.passive=1;p.itvl=160;p.window=48;p.filter_duplicates=0;e=ble_gap_disc(type,BLE_HS_FOREVER,&p,gap,nullptr);}
 portENTER_CRITICAL(&guard);scanError=e;scanning=e==0;portEXIT_CRITICAL(&guard);
}
void host(void*){nimble_port_run();nimble_port_freertos_deinit();}
}
namespace {
void initialize(){
 bool passed=test();int e=0;
#if CONFIG_IDF_TARGET_ESP32P4
 auto config=xtouch_hosted_bt_config(!XTOUCH_P4_C6_PREINITIALIZED_BT);
 e=esp_hosted_bt_host_stack_setup(&config);
 Serial.printf("[XIAOMI-BLE] Hosted 3 NimBLE binding=%d\n",e);
#endif
 if(!e){e=nimble_port_init();Serial.printf("[XIAOMI-BLE] host init=%d\n",e);}
 portENTER_CRITICAL(&guard);selftest=passed;scanError=e;portEXIT_CRITICAL(&guard);if(e)return;
 ble_hs_cfg.sync_cb=onSync;nimble_port_freertos_init(host);
}
#if CONFIG_IDF_TARGET_ESP32P4
void initializeTask(void*){initialize();vTaskDelete(nullptr);}
#endif
}
void xtouch_xiaomi_ble_begin(){
 xtouch_environment_begin();
#if CONFIG_IDF_TARGET_ESP32P4
 if(xTaskCreate(initializeTask,"xiaomi-ble",6144,nullptr,2,nullptr)!=pdPASS){portENTER_CRITICAL(&guard);scanError=ESP_ERR_NO_MEM;portEXIT_CRITICAL(&guard);}
#else
 initialize();
#endif
}

void xtouch_xiaomi_ble_status(JsonObject out){
 Device snapshot[8];uint32_t total,bad,ignored,full;bool active,passed;int error;
 portENTER_CRITICAL(&guard);memcpy(snapshot,devices,sizeof(devices));total=packets;bad=invalid;ignored=other;full=overflow;active=scanning;passed=selftest;error=scanError;portEXIT_CRITICAL(&guard);

 out["kind"]="xiaomi-ble";out["selftest"]=passed;out["scanning"]=active;out["scanError"]=error;out["packets"]=total;out["malformed"]=bad;out["other"]=ignored;out["overflow"]=full;
 uint8_t configuredMac[6];portENTER_CRITICAL(&guard);bool hasKey=bound&&keyPresent;uint32_t failed=authFailures;memcpy(configuredMac,bindingMac,6);portEXIT_CRITICAL(&guard);
 out["passive"]=true;out["scanDutyPercent"]=30;out["keyConfigured"]=hasKey;out["authenticationFailures"]=failed;
 auto list=out.createNestedArray("sensors");
 for(auto &v:snapshot)if(v.used){auto item=list.createNestedObject();char mac[18];snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",v.mac[5],v.mac[4],v.mac[3],v.mac[2],v.mac[1],v.mac[0]);item["model"]=xiaomi_ble::model_name(v.reading.product);item["mac"]=mac;item["rssi"]=v.rssi;item["packets"]=v.packets;item["encrypted"]=v.encrypted;item["ageMs"]=millis()-v.lastMs;item["authenticated"]=v.authenticated;item["needsBindkey"]=v.reading.encrypted&&(!hasKey||memcmp(configuredMac,v.mac,6));if(v.reading.temperature){item["temperature"]=v.reading.t;item["temperatureAgeMs"]=millis()-v.tAt;}if(v.reading.humidity){item["humidity"]=v.reading.h;item["humidityAgeMs"]=millis()-v.hAt;}}
}
bool xtouch_xiaomi_ble_bind(const char *mac,const char *key){
 if(!mac||strlen(mac)!=17||(key&&*key&&strlen(key)!=32))return false;
 bool supplied=key&&*key;
 auto hex=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
 uint8_t m[6],k[16]={};
 for(int i=0;i<6;++i){int a=hex(mac[3*i]),b=hex(mac[3*i+1]);if(a<0||b<0||(i<5&&mac[3*i+2]!=':'))return false;m[5-i]=(a<<4)|b;}
 if(supplied)for(int i=0;i<16;++i){int a=hex(key[2*i]),b=hex(key[2*i+1]);if(a<0||b<0){memset(k,0,sizeof(k));return false;}k[i]=(a<<4)|b;}
 portENTER_CRITICAL(&guard);memcpy(bindingMac,m,6);memcpy(bindingKey,k,16);bound=true;keyPresent=supplied;authFailures=0;++bindingRevision;
 for(auto &v:devices)if(v.used&&!memcmp(v.mac,m,6)){v.reading={};v.authenticated=false;v.tAt=v.hAt=0;}
 portEXIT_CRITICAL(&guard);memset(k,0,sizeof(k));return true;
}

void xtouch_xiaomi_ble_unbind(){portENTER_CRITICAL(&guard);bound=false;keyPresent=false;++bindingRevision;memset(bindingKey,0,sizeof(bindingKey));memset(bindingMac,0,sizeof(bindingMac));for(auto &v:devices)v={};portEXIT_CRITICAL(&guard);}
bool xtouch_xiaomi_ble_measurement(float &t,float &h,uint32_t &age,uint32_t *temperatureAge,uint32_t *humidityAge,uint32_t *seenAge){bool found=false;uint32_t now=millis();portENTER_CRITICAL(&guard);if(bound)for(const auto &v:devices)if(v.used&&(v.authenticated||(!v.reading.encrypted&&!xiaomi_ble::requires_encryption(v.reading.product)))&&!memcmp(v.mac,bindingMac,6)&&v.reading.temperature&&v.reading.humidity){t=v.reading.t;h=v.reading.h;age=max(now-v.tAt,now-v.hAt);if(temperatureAge)*temperatureAge=now-v.tAt;if(humidityAge)*humidityAge=now-v.hAt;if(seenAge)*seenAge=now-v.lastMs;found=true;break;}portEXIT_CRITICAL(&guard);return found;}

const char *xtouch_xiaomi_ble_model(){uint16_t product=0;portENTER_CRITICAL(&guard);if(bound)for(const auto &v:devices)if(v.used&&!memcmp(v.mac,bindingMac,6)){product=v.reading.product;break;}portEXIT_CRITICAL(&guard);return xiaomi_ble::model_name(product);}

void xtouch_xiaomi_ble_model_status(JsonObject out){uint16_t product=0;portENTER_CRITICAL(&guard);if(bound)for(const auto &v:devices)if(v.used&&!memcmp(v.mac,bindingMac,6)){product=v.reading.product;break;}portEXIT_CRITICAL(&guard);auto m=xiaomi_ble::model(product);out["model"]=m?m->code:"";if(m){out["manufacturer"]=m->maker;out["productId"]=product;}}

bool xtouch_xiaomi_ble_key_required(){bool result=false;portENTER_CRITICAL(&guard);if(bound&&!keyPresent)for(const auto &v:devices)if(v.used&&!memcmp(v.mac,bindingMac,6)&&(v.reading.encrypted||xiaomi_ble::requires_encryption(v.reading.product))){result=true;break;}portEXIT_CRITICAL(&guard);return result;}
#endif
