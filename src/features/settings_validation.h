#pragma once
#include <ArduinoJson.h>
#include "ui/i18n/language_metadata.h"
#include "xtouch/clock_settings.h"
inline bool xtouch_feature_settings_valid(JsonVariantConst doc){
 if(!doc.isNull()&&!doc.is<JsonObjectConst>())return false;
 for(const char *key:{"tftInvert","printerPickerList","ota","wop","chamberTemp","auxFan","chamberFan","touchSound","warningSound","statusLed","browserTimezoneSync"})if(doc.containsKey(key)&&!doc[key].is<bool>())return false;
 for(const char *key:{"backlight","tftOff","chamberTempDiff"})if(doc.containsKey(key)){
  const int low=!strcmp(key,"chamberTempDiff")?-128:0;
  const int high=!strcmp(key,"backlight")?255:!strcmp(key,"tftOff")?240:127;
  if(!doc[key].is<int>()||doc[key].as<int>()<low||doc[key].as<int>()>high)return false;
 }
 for(const char *key:{"language","portalLanguage"})if(doc.containsKey(key)){
  if(!doc[key].is<const char*>())return false;
  bool supported=false;for(const char *code:{XTOUCH_LANGUAGE_CODES})supported|=*code&&!strcmp(doc[key],code);
  if(!supported)return false;
 }
 if(doc.containsKey("soundVolume")&&(!doc["soundVolume"].is<int>()||doc["soundVolume"].as<int>()<0||doc["soundVolume"].as<int>()>100))return false;
 if(doc.containsKey("deepSleepAfter")&&(!doc["deepSleepAfter"].is<int>()||doc["deepSleepAfter"].as<int>()<0||doc["deepSleepAfter"].as<int>()>240))return false;
 if(doc.containsKey("timezone")&&(!doc["timezone"].is<const char*>()||!XTouchClock::timezone(doc["timezone"])))return false;
 if(doc.containsKey("ntpServer")){if(!doc["ntpServer"].is<const char*>())return false;const char *server=doc["ntpServer"];if(*server&&!XTouchClock::server(server))return false;}
 if(doc.containsKey("timezoneName")){if(!doc["timezoneName"].is<const char*>()||strlen(doc["timezoneName"])>=64)return false;
  for(const char *p=doc["timezoneName"];*p;++p)if(!isalnum((unsigned char)*p)&&*p!='/'&&*p!='_'&&*p!='-'&&*p!='+')return false;}
 return true;
}
