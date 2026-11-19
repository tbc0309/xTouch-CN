#pragma once
#include <ArduinoJson.h>
#include "vault.h"

namespace XTouchCredentials {
inline String lanBinding(JsonVariantConst printer) {
 return String("lan:")+(printer["serialNumber"]|"")+":"+(printer["host"]|"");
}
inline bool accessCodeValid(const String &plain) {
 if(plain.length()!=8)return false;
 for(size_t i=0;i<plain.length();++i)if((uint8_t)plain[i]<33||(uint8_t)plain[i]>126)return false;
 return true;
}
inline bool accessCodeEncrypted(JsonVariantConst printer) {
 return printer["accessCodeEncrypted"]|bool(strlen(printer["accessCodeEnc"]|""));
}
inline bool readAccessCode(JsonVariantConst printer,String &plain) {
 plain=printer["accessCode"]|"";
 if(plain.length())return accessCodeValid(plain);
 const char *sealed=printer["accessCodeEnc"]|"";
 return *sealed&&xtouch_vault_open(sealed,lanBinding(printer),plain)&&accessCodeValid(plain);
}
inline bool storeAccessCode(JsonObject printer,const String &plain) {
 if(!accessCodeValid(plain))return false;
 const bool encrypted=accessCodeEncrypted(printer);
 String sealed;if(encrypted){sealed=xtouch_vault_seal(plain,lanBinding(printer));if(sealed.isEmpty())return false;}
 printer["accessCode"]=encrypted?String():plain;
 if(encrypted)printer["accessCodeEnc"]=sealed;else printer.remove("accessCodeEnc");
 return !strcmp(printer["accessCode"]|"",encrypted?"":plain.c_str())&&!strcmp(printer["accessCodeEnc"]|"",sealed.c_str());
}
inline bool mergeAccessCode(JsonObject printer,JsonVariantConst previous) {
 if(!previous.isNull() && lanBinding(printer)!=lanBinding(previous)) {
  String plain=printer["accessCode"]|"";
  if(plain.isEmpty()&&!strlen(printer["accessCodeEnc"]|"")&&!readAccessCode(previous,plain))return false;
  if(plain.length())return storeAccessCode(printer,plain);
 }

 if(strlen(printer["accessCode"]|"")) {
  String plain=printer["accessCode"].as<String>(),old;
  const bool encrypted=accessCodeEncrypted(printer);
  const bool sameFormat=encrypted?(!strlen(previous["accessCode"]|"")&&strlen(previous["accessCodeEnc"]|"")):!strlen(previous["accessCodeEnc"]|"");
  if(encrypted==accessCodeEncrypted(previous)&&sameFormat&&readAccessCode(previous,old)&&old==plain){
   printer["accessCode"]=previous["accessCode"].as<String>();
   if(strlen(previous["accessCodeEnc"]|""))printer["accessCodeEnc"]=previous["accessCodeEnc"];else printer.remove("accessCodeEnc");
   return true;
  }
  return storeAccessCode(printer,plain);
 }
 if(!strlen(printer["accessCodeEnc"]|"")) {
  printer["accessCode"]=previous["accessCode"].as<String>();
  if(strlen(previous["accessCodeEnc"]|""))printer["accessCodeEnc"]=previous["accessCodeEnc"];
 }
 if(accessCodeEncrypted(printer)&&strlen(printer["accessCodeEnc"]|"")){printer["accessCode"]="";return true;}
 String plain;if(!readAccessCode(printer,plain))return false;
 return storeAccessCode(printer,plain);
}
inline String binding(JsonVariantConst account) {
 return String(account["id"]|"")+":"+(account["region"]|"cn")+":"+(account["account"]|"");
}
inline bool read(JsonVariantConst account,bool token,String &plain) {
 plain=account[token?"token":"password"]|"";
 if(plain.length())return true;
 const char *sealed=account[token?"tokenEnc":"passwordEnc"]|"";
 if(*sealed)return xtouch_vault_open(sealed,(token?String("token:"):String())+binding(account),plain);
 return false;
}
inline bool encrypted(JsonVariantConst account,bool token) {
 return account[token?"tokenEncrypted":"passwordEncrypted"]|bool(strlen(account[token?"tokenEnc":"passwordEnc"]|""));
}
inline bool store(JsonObject account,bool token,const String &plain) {
 const char *field=token?"token":"password",*encryptedField=token?"tokenEnc":"passwordEnc";
 const bool encrypted=XTouchCredentials::encrypted(account,token);
 String sealed;
 if(plain.length()&&encrypted){sealed=xtouch_vault_seal(plain,(token?String("token:"):String())+binding(account));if(sealed.isEmpty())return false;}
 account[field]=encrypted?String():plain;
 if(sealed.length())account[encryptedField]=sealed;else account.remove(encryptedField);
 return !strcmp(account[field]|"",encrypted?"":plain.c_str()) &&
        !strcmp(account[encryptedField]|"",sealed.c_str());
}
inline bool merge(JsonObject account,JsonVariantConst previous,bool token) {
 const char *field=token?"token":"password",*sealed=token?"tokenEnc":"passwordEnc";
 if(account["clearToken"].as<bool>()) {account[field]="";account.remove(sealed);return true;}
 if(!previous.isNull() && binding(account)!=binding(previous) && !account["clearToken"].as<bool>()) {
  String plain=account[field]|"";
  if(plain.isEmpty()&&!strlen(account[sealed]|"")&&!read(previous,token,plain))return !strlen(previous[sealed]|"")&&store(account,token,String());
  if(plain.length())return store(account,token,plain);
 }
 if(strlen(account[field]|"")) {
  String plain=account[field].as<String>(),old;
  const bool cipher=encrypted(account,token);
  const bool sameFormat=cipher?(!strlen(previous[field]|"")&&strlen(previous[sealed]|"")):!strlen(previous[sealed]|"");
  if(cipher==encrypted(previous,token)&&sameFormat&&read(previous,token,old)&&plain==old){
   account[field]=previous[field].as<String>();
   if(strlen(previous[sealed]|""))account[sealed]=previous[sealed];else account.remove(sealed);
   return true;
  }
  return store(account,token,plain);
 }
 if(!strlen(account[sealed]|"")) {
  account[field]=String(previous[field]|"");
  if(strlen(previous[sealed]|""))account[sealed]=previous[sealed];
 }
 const bool encrypted=XTouchCredentials::encrypted(account,token);
 if(encrypted&&strlen(account[sealed]|"")) {account[field]="";return true;}
 String plain;
 if(!read(account,token,plain))return !strlen(account[sealed]|"");
 return store(account,token,plain);
}
}
