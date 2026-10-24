#include "cloud.h"
#include "cloud_endpoints.h"
#include <memory>
#include "cloud_ca.h"
#include "cloud_response.h"
#include "storage.h"
#include "vault.h"
#include "credentials.h"
#include "lan_probe.h"
#include "profile_schema.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <mbedtls/base64.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
String activeToken, activeUser, activeRegion, activeError;
String labBroker;
uint16_t labPort=8883;
bool labActive=false;
String jobInput, jobOutput;
volatile bool running=false, pending=false;
portMUX_TYPE jobMux=portMUX_INITIALIZER_UNLOCKED;
uint32_t jobFinished=0;
const char *api(const char *region) {auto *endpoint=XTouchCloudEndpoints::get(region);return endpoint?endpoint->api:"";}
void wipe(String &s) { for(size_t i=0;i<s.length();++i)s.setCharAt(i,0);s=""; }
class LimitedBody : public Stream {
public:
 String body; bool overflow=false;
 size_t write(uint8_t c) override { if(body.length()>=16384){overflow=true;return 0;}return body.concat((char)c)?1:0; }
 size_t write(const uint8_t *p,size_t n) override {size_t room=16384-body.length(),take=n<room?n:room;if(take<n)overflow=true;return take&&body.concat((const char *)p,take)?take:0; }
 int available() override{return 0;} int read()override{return -1;} int peek()override{return -1;} void flush()override{}
};
bool uidFromToken(const String &token,String &uid) {
 int a=token.indexOf('.'),b=token.indexOf('.',a+1);if(a<1||b<=a+1||b-a>4096)return false;
 String payload=token.substring(a+1,b);payload.replace('-','+');payload.replace('_','/');while(payload.length()%4)payload+='=';
 size_t n=0;mbedtls_base64_decode(nullptr,0,&n,(const unsigned char *)payload.c_str(),payload.length());
 if(!n||n>4096)return false;auto decoded=std::unique_ptr<unsigned char[]>(new(std::nothrow) unsigned char[n+1]);if(!decoded)return false;
 if(mbedtls_base64_decode(decoded.get(),n,&n,(const unsigned char *)payload.c_str(),payload.length()))return false;decoded[n]=0;
 DynamicJsonDocument doc(6144);if(deserializeJson(doc,decoded.get(),n))return false;
 uid=doc["uid"].as<String>();if(uid=="null"||!uid.length())uid=doc["sub"].as<String>();
 if(uid=="null"||!uid.length())uid=doc["user_id"].as<String>();
 if(uid=="null"||!uid.length()||uid.length()>40)return false;
 if(!uid.startsWith("u_"))uid="u_"+uid;
 return true;
}
String webCsrf,webRegion;
uint32_t authBlockedAt[4]={};
bool authBlocked[4]={};
unsigned authRetryAfter=0;
int challengeStatus=0,responseStatus=0,lastReadError=0;
unsigned lastBodyBytes=0;
int request(const String &url,const String &token,const String &payload,String &reply) {
 reply="";responseStatus=0;lastReadError=0;lastBodyBytes=0;challengeStatus=0;authRetryAfter=0;
 if(time(nullptr)<1577836800)return -10;
 bool website=url.startsWith("https://bambulab.cn/")||url.startsWith("https://bambulab.com/");
 String site=url.startsWith("https://bambulab.cn/")?"https://bambulab.cn":"https://bambulab.com";
 if(website&&webRegion!=site){webCsrf="";webRegion=site;}
 const bool auth=token.isEmpty();
 const unsigned origin=(url.startsWith("https://bambulab.cn/")?0:url.startsWith("https://bambulab.com/")?1:url.startsWith("https://api.bambulab.cn/")?2:3);
 authRetryAfter=0;
 if(auth&&authBlocked[origin]){
  uint32_t elapsed=millis()-authBlockedAt[origin];
  if(elapsed<300000){authRetryAfter=(300000-elapsed+999)/1000;challengeStatus=0;return -12;}
  authBlocked[origin]=false;
 }
 if(website&&payload.length()){
  wipe(webCsrf);String unused;int csrfCode=request(site+"/api/csrf","","",unused);
  if(csrfCode<200||csrfCode>=300)return csrfCode;
  if(webCsrf.isEmpty())return -13;
 }
 WiFiClientSecure tls;tls.setCACert(xtouch_cloud_ca);tls.setHandshakeTimeout(6);tls.setTimeout(6);
 HTTPClient http;http.setConnectTimeout(6000);http.setTimeout(12000);http.setReuse(false);
 if(!http.begin(tls,url))return -1;
 const char *keys[]={"Set-Cookie","cf-mitigated","cf-ray"};http.collectHeaders(keys,3);
 http.addHeader("Accept","application/json");
 if(website){
  if(webCsrf.length()){http.addHeader("Cookie","bbl_csrf_token="+webCsrf);if(payload.length())http.addHeader("x-bbl-csrf-token",webCsrf);}}

 http.addHeader("User-Agent","xTouch-CN/1.0 (+https://github.com/tbc0309/xTouch-CN)");http.addHeader("Content-Type","application/json");
 if(token.length())http.addHeader("Authorization","Bearer "+token);
 int code=payload.length()?http.POST((uint8_t *)payload.c_str(),payload.length()):http.GET();
 if(website&&url.endsWith("/api/csrf")){
  String cookie=http.header("Set-Cookie");int start=cookie.indexOf("bbl_csrf_token=");if(start>=0){int end=cookie.indexOf(';',start);String value=cookie.substring(start+15,end<0?cookie.length():end);if(value.length()<256)webCsrf=value;}
 }
 int status=code;responseStatus=code;lastReadError=0;lastBodyBytes=0;
 // These statuses have no response body; a closed TLS socket is normal.
 if(xtouch_cloud_has_body(code,http.getSize())) {LimitedBody sink;int got=http.writeToStream(&sink);reply=std::move(sink.body);lastReadError=got<0?got:0;lastBodyBytes=reply.length();if(got<0||sink.overflow){Serial.printf("[CLOUD receive] HTTP=%d stream=%d stored=%u overflow=%d\n",status,got,unsigned(reply.length()),int(sink.overflow));code=sink.overflow?-11:-14;}}
 if(xtouch_cloud_challenge(status,reply.c_str(),http.header("cf-mitigated").c_str(),http.header("cf-ray").c_str())){
  challengeStatus=status;
  if(auth){authBlocked[origin]=true;authBlockedAt[origin]=millis();authRetryAfter=300;}
  code=-12;
 }
 http.end();return code;
}
String executeJob() {
 DynamicJsonDocument input(6144);deserializeJson(input,jobInput);wipe(jobInput);
 String mode=input["action"]|"",region=input["region"]|"cn",token=input["token"]|"";
 if(mode=="validate_lan")return XTouchLanProbe::run(input.as<JsonVariantConst>());
 DynamicJsonDocument out(4096);
 String body,reply;int code=-1;
 String remembered;
 if(mode=="login"&&input["rememberPassword"].as<bool>())remembered=xtouch_vault_seal(input["password"]|"",String(input["accountId"]|"")+":"+region+":"+(input["account"]|""));
 if(mode=="login" || mode=="login_code" || mode=="login_tfa") {
  DynamicJsonDocument login(1024);login["account"]=input["account"];if(mode=="login")login["password"]=input["password"];else login["code"]=input["code"];if(mode=="login_tfa"){login.clear();login["tfaKey"]=input["tfaKey"];login["tfaCode"]=input["code"];}serializeJson(login,body);input.clear();input.shrinkToFit();login.clear();login.shrinkToFit();
  code=request(mode=="login_tfa"?String(XTouchCloudEndpoints::get(region.c_str())->website)+"/api/sign-in/tfa":String(api(region.c_str()))+"/v1/user-service/user/login","",body,reply);wipe(body);
  DynamicJsonDocument data(6144);
  if(code==200&&!deserializeJson(data,reply)) {
   token=data["accessToken"]|""; if(token.isEmpty())token=data["token"]|"";
   if(token.isEmpty()){token=data["data"]["accessToken"]|"";if(token.isEmpty())token=data["data"]["token"]|"";}
   if(!token.isEmpty()&&token.length()<=2048) {
    String uid;bool found=uidFromToken(token,uid);data.clear();data.shrinkToFit();wipe(reply);
    if(!found){code=request(String(api(region.c_str()))+"/v1/user-service/my/profile",token,"",reply);DynamicJsonDocument profile(4096);
      if(code==200&&!deserializeJson(profile,reply)){uid=profile["uidStr"].as<String>();if(uid=="null"||uid.isEmpty())uid=profile["uid"].as<String>();found=uid!="null"&&!uid.isEmpty()&&uid.length()<40;}}
    if(found){out["token"]=token;out["userId"]=uid.startsWith("u_")?uid:"u_"+uid;out["ok"]=true;if(remembered.length())out["passwordEnc"]=remembered;}
   }
   if(!out["ok"].as<bool>()) {
    const char *kind=data["loginType"]|"";if(strlen(data["tfaKey"]|"")<=512)out["tfaKey"]=data["tfaKey"];
    out["error"]=(!strcmp(kind,"tfa")||strlen(data["tfaKey"]|""))?"two_factor_required":"verification_required";
   }
  }
 } else if(mode=="send_code") {
  bool phone=!strchr(input["account"]|"",'@');
  DynamicJsonDocument codeBody(512);codeBody[phone?"phone":"email"]=input["account"];codeBody["type"]="codeLogin";serializeJson(codeBody,body);
  input.clear();input.shrinkToFit();
  String url=phone?"https://bambulab.cn/api/v1/user-service/user/sendsmscode":String(api(region.c_str()))+"/v1/user-service/user/sendemail/code";
  code=request(url,"",body,reply);wipe(body);DynamicJsonDocument response(1024);
  if(code>=200&&code<300&&(reply.isEmpty()||(!deserializeJson(response,reply)&&!response["error"].is<const char *>()&&(!response.containsKey("success")||response["success"].as<bool>())&&(!response.containsKey("code")||response["code"].as<int>()==0))))out["ok"]=true;
 } else if(mode=="devices") {
  input.clear();input.shrinkToFit();
  const auto *endpoint=XTouchCloudEndpoints::get(region.c_str());
  code=request(String(endpoint->website)+"/api/v1/iot-service/api/user/bind",token,"",reply);
  // The official website proxy and API can have different edge protections.
  // Both routes use this account's region and token; never cross regions.
  if(code==-12||code==404)code=request(String(endpoint->api)+"/v1/iot-service/api/user/bind",token,"",reply);
  out=DynamicJsonDocument(min((size_t)16384,max((size_t)2048,reply.length()*2)));
  DynamicJsonDocument data(min((size_t)24576,max((size_t)2048,reply.length()*2)));
  if(code==200&&!deserializeJson(data,reply)) {
   JsonArray devices=out.createNestedArray("devices");unsigned count=0;
   for(JsonVariantConst device:data["devices"].as<JsonArrayConst>()) {
    if(count++>=100)break;JsonObject p=devices.createNestedObject();p["serialNumber"]=device["dev_id"];p["name"]=device["name"];p["printerModel"]=device["dev_product_name"];p["online"]=device["online"].as<bool>();
   }
   out["ok"]=true;
  }
 } else {
  input.clear();input.shrinkToFit();
  code=request(String(api(region.c_str()))+"/v1/user-service/my/profile",token,"",reply);
  DynamicJsonDocument data(8192);
  if(code==200&&!deserializeJson(data,reply)) {
   String uid=data["uidStr"].as<String>();if(uid=="null"||uid.isEmpty())uid=data["uid"].as<String>();
   if(uid!="null"&&!uid.isEmpty()&&uid.length()<40){out["userId"]=uid.startsWith("u_")?uid:"u_"+uid;out["ok"]=true;}
  }
 }
 if(remembered.length() && (out["ok"].as<bool>() || !strcmp(out["error"]|"","verification_required") || !strcmp(out["error"]|"","two_factor_required")))out["passwordEnc"]=remembered;
 if(!out["ok"].as<bool>()&&!out.containsKey("error")) {
  out["ok"]=false;
  String check=reply;check.toLowerCase();
  out["error"]=code==-12?"cloud_blocked":code==-13?"csrf_required":code==-10?"clock_not_ready":code==-11?"response_too_large":code<0?"cloud_network_error":
   ((code==403||code==429)&&check.indexOf("cloudflare")>=0)?"cloud_blocked":
   (code==403&&(check.indexOf("missing_cookie")>=0||check.indexOf("missing_header")>=0))?"csrf_required":
   code==401?"authentication_rejected":code==403?"cloud_forbidden":code==429?"cloud_rate_limited":"cloud_request_failed";
 }
 Serial.printf("[CLOUD] action=%s HTTP=%d result=%d responseBytes=%u heap=%u\n",mode.c_str(),code==-12?challengeStatus:(code==-11||code==-14)?responseStatus:code,code,unsigned(reply.length()),unsigned(ESP.getFreeHeap()));
 if(out.overflowed()){out.clear();out["ok"]=false;out["error"]="response_too_large";}
 out["httpStatus"]=code==-12?challengeStatus:(code==-11||code==-14)?responseStatus:code>0?code:0;if(lastReadError)out["receiveError"]=lastReadError;out["responseBytes"]=lastBodyBytes;if(code==-12&&authRetryAfter)out["retryAfter"]=authRetryAfter;input.clear();wipe(token);wipe(reply);
 String result;serializeJson(out,result);out.clear();
 return result; // Return normally so every String/JSON/TLS owner is destroyed.
}
void job(void *) {
 { String result=executeJob();jobOutput=std::move(result); }
 Serial.printf("[CLOUD] released heap=%u minStack=%u\n",unsigned(ESP.getFreeHeap()),unsigned(uxTaskGetStackHighWaterMark(nullptr)));
 portENTER_CRITICAL(&jobMux);running=false;pending=false;jobFinished=millis();portEXIT_CRITICAL(&jobMux);
 vTaskDelete(nullptr);
}
}
bool xtouch_cloud_select(const char *id) {
 wipe(activeToken);activeUser="";activeRegion="";activeError="";
 labActive=false;labBroker="";labPort=8883;
 if(!id||!*id)return true;
 DynamicJsonDocument doc=xtouch_storage_read("/xtouch.json",0);
 for(JsonVariantConst account:doc["cloudAccounts"].as<JsonArrayConst>())if(!strcmp(account["id"]|"",id)) {
  if(!XTouchSchema::account(account)){activeError="invalid_configuration";return false;}
  if(account["simulator"].as<bool>()){
   labBroker=account["mqttHost"].as<String>();labPort=account["mqttPort"].as<unsigned>();labActive=true;
   activeUser="xtouch-lab";activeToken="xtouch-lab";return true;
  }
  if(account["rememberPassword"].as<bool>()&&strlen(account["passwordEnc"]|"")){
   String password;if(!XTouchCredentials::read(account,false,password)){activeError="password_key_changed";return false;}
  }
  if(!XTouchCredentials::read(account,true,activeToken)){activeError="credential_key_changed";return false;}activeRegion=account["region"]|"cn";activeUser=account["userId"]|"";
  if(!activeUser.length())uidFromToken(activeToken,activeUser);
  if(activeToken.length()>2048||activeUser.length()>47){wipe(activeToken);activeUser="";}
  if(activeUser.length()&&!activeUser.startsWith("u_"))activeUser="u_"+activeUser;
  break;
 }
 if(activeToken.isEmpty()||activeUser.isEmpty())activeError="cloud_account_invalid";
 return activeError.isEmpty();
}
const char *xtouch_cloud_broker(){auto *endpoint=XTouchCloudEndpoints::get(activeRegion.c_str());return labActive?labBroker.c_str():endpoint?endpoint->mqtt:"";}
bool xtouch_cloud_simulator(){return labActive;}
uint16_t xtouch_cloud_port(){return labActive?labPort:8883;}
const char *xtouch_cloud_user(){return activeUser.c_str();}
const char *xtouch_cloud_token(){return activeToken.c_str();}
const char *xtouch_cloud_error(){return activeError.c_str();}
const char *xtouch_cloud_trust(){return xtouch_cloud_ca;}
bool xtouch_cloud_ready(){return activeError.isEmpty()&&activeToken.length()&&activeUser.length();}
bool xtouch_cloud_busy(){return running;}
bool xtouch_cloud_job_pending(){return pending;}
bool xtouch_cloud_job_start(JsonVariantConst r) {
 if(running||measureJson(r)>5500)return false;
 const char *action=r["action"]|"";bool lan=!strcmp(action,"validate_lan");if(!lan&&strcmp(action,"login")&&strcmp(action,"login_code")&&strcmp(action,"send_code")&&strcmp(action,"devices")&&strcmp(action,"validate")&&strcmp(action,"login_tfa"))return false;
 if(lan&&!XTouchSchema::lanCandidate(r))return false;
 const char *region=r["region"]|"cn";if(strcmp(region,"cn")&&strcmp(region,"global"))return false;
 if(!strcmp(action,"login")&&(!r["account"].is<const char *>()||strlen(r["account"])>100||!r["password"].is<const char *>()||strlen(r["password"])>128))return false;
 bool accountAction=!strcmp(action,"login")||!strcmp(action,"login_code")||!strcmp(action,"send_code")||!strcmp(action,"login_tfa");
 if(accountAction&&(!r["account"].is<const char *>()||!strlen(r["account"])||strlen(r["account"])>100))return false;
 if(!strcmp(action,"login")&&!strlen(r["password"]|""))return false;
 if(!strcmp(action,"login_tfa")&&(!strlen(r["tfaKey"]|"")||strlen(r["tfaKey"]|"")>512))return false;
 if(!strcmp(action,"login_code")||!strcmp(action,"login_tfa")){const char *code=r["code"]|"";if(strlen(code)!=6)return false;for(const char *p=code;*p;++p)if(*p<'0'||*p>'9')return false;}
 if(!strcmp(action,"send_code")&&!strchr(r["account"]|"",'@')&&strcmp(region,"cn"))return false;
 if(!accountAction&&!lan&&(!r["token"].is<const char *>()||!strlen(r["token"])||strlen(r["token"])>2048))return false;
 wipe(jobOutput);serializeJson(r,jobInput);running=true;pending=true;
 // The main loop releases printer TLS before the worker is started by its next poll.
 return true;
}
String xtouch_cloud_job_result(){
 if(pending&&running) {
  pending=false;
  if(xTaskCreatePinnedToCore(job,"xt-cloud",12288,nullptr,1,nullptr,0)!=pdPASS){running=false;wipe(jobInput);jobOutput="{\"ok\":false,\"error\":\"out_of_memory\"}";}
 }
 if(running)return "{\"busy\":true}";
 if(jobFinished&&millis()-jobFinished>120000)wipe(jobOutput);
 return jobOutput.length()?jobOutput:"{\"idle\":true}";
}
void xtouch_cloud_job_clear(){if(!running){wipe(jobInput);wipe(jobOutput);}}

bool xtouch_cloud_preview_credentials(char *token,size_t capacity,bool &china){
 if(!token||capacity<2)return false;token[0]=0;
 if(labActive||!xtouch_cloud_ready()||!XTouchCloudEndpoints::get(activeRegion.c_str())||activeToken.length()>=capacity)return false;
 memcpy(token,activeToken.c_str(),activeToken.length()+1);china=activeRegion=="cn";return true;
}
