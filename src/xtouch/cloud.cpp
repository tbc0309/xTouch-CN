#include "cloud_memory.h"
#include "features/network/task_budget.h"
#include "cloud_cookies.h"
#include "features/config.h"
#include "features/network/limited_body.h"
#include "cloud.h"
#include "cloud_endpoints.h"
#include <memory>
#include "cloud_ca.h"
#include "cloud_response.h"
#if XTOUCH_FEATURE_CLOUD_LOGIN
#include "cloud_login.h"
#endif
#if XTOUCH_FEATURE_CLOUD_TOKEN
#include "cloud_token.h"
#endif
#include "storage.h"
#include "vault.h"
#include "credentials.h"
#include "lan_probe.h"
#include "profile_schema.h"
#include <esp_http_client.h>
#include <mbedtls/base64.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
template<class Setter> void clientHeaders(Setter set){
 set("User-Agent","bambu_network_agent/01.09.05.01");
 set("X-BBL-Client-Name","OrcaSlicer");
 set("X-BBL-Client-Type","slicer");
 set("X-BBL-Client-Version","01.09.05.51");
 set("X-BBL-Language","en-US");
 set("X-BBL-OS-Type","linux");
 set("X-BBL-OS-Version","6.2.0");
 set("X-BBL-Agent-Version","01.09.05.01");
 set("X-BBL-Executable-info","{}");
 set("X-BBL-Agent-OS-Type","linux");
 set("Content-Type","application/json");
}
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
using LimitedBody=XTouchLimitedBody<16384>;
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
 struct Response {
  LimitedBody body;XTouchCloudCookies::Headers cookies;String mitigated,ray;
 } response;
 int status=0,code=-1;
 esp_http_client_config_t config={};config.url=url.c_str();config.cert_pem=xtouch_cloud_ca;
 config.timeout_ms=12000;config.keep_alive_enable=false;config.buffer_size=2048;config.buffer_size_tx=1024;
 config.method=payload.length()?HTTP_METHOD_POST:HTTP_METHOD_GET;config.user_data=&response;
 config.event_handler=[](esp_http_client_event_t *event)->esp_err_t {
  auto *r=static_cast<Response *>(event->user_data);
  if(event->event_id==HTTP_EVENT_ON_DATA && event->data_len>0){
   if(r->body.write(static_cast<const uint8_t *>(event->data),event->data_len)!=size_t(event->data_len))return ESP_FAIL;
   if(r->body.overflow)return ESP_FAIL;
  }else if(event->event_id==HTTP_EVENT_ON_HEADER && event->header_key && event->header_value){
   if(!strcasecmp(event->header_key,"Set-Cookie"))r->cookies.add(event->header_value);
   else if(!strcasecmp(event->header_key,"cf-mitigated"))r->mitigated=event->header_value;
   else if(!strcasecmp(event->header_key,"cf-ray"))r->ray=event->header_value;
  }
  return ESP_OK;
 };
 esp_http_client_handle_t http=esp_http_client_init(&config);if(!http)return -1;
 esp_http_client_set_header(http,"Accept","application/json");
 if(website&&webCsrf.length()){
  String cookie="bbl_csrf_token="+webCsrf;esp_http_client_set_header(http,"Cookie",cookie.c_str());
  if(payload.length())esp_http_client_set_header(http,"x-bbl-csrf-token",webCsrf.c_str());
 }
 clientHeaders([&](const char *key,const char *value){esp_http_client_set_header(http,key,value);});
 if(token.length()){String bearer="Bearer "+token;esp_http_client_set_header(http,"Authorization",bearer.c_str());}
 if(payload.length())esp_http_client_set_post_field(http,payload.c_str(),payload.length());
 esp_err_t error=esp_http_client_perform(http);status=esp_http_client_get_status_code(http);
 // IDF's body dispatcher does not propagate an event handler's return value.
 if(response.body.failed&&!response.body.overflow&&error==ESP_OK)error=ESP_ERR_NO_MEM;
 responseStatus=status;lastReadError=error==ESP_OK?0:-int(error);
 reply=std::move(response.body.body);lastBodyBytes=reply.length();
 code=response.body.overflow?-11:error==ESP_OK?status:status>0?-14:-1;
 esp_http_client_cleanup(http);
 if(website&&url.endsWith("/api/csrf")&&!response.cookies.csrf.empty())webCsrf=response.cookies.csrf.c_str();
 /* The website TFA endpoint can deliver the session only as Set-Cookie.
    Use it solely after a successful response from that exact endpoint. */
 if(code==200&&website&&url.endsWith("/api/sign-in/tfa")&&!response.cookies.token.empty()){
  DynamicJsonDocument auth(6144);
  bool valid=reply.isEmpty()||!deserializeJson(auth,reply);
  if(valid&&(auth.isNull()||auth.is<JsonObject>())){
   if(auth.isNull())auth.to<JsonObject>();
   if(!(auth["accessToken"]|"")[0]&&!(auth["token"]|"")[0]&&!(auth["data"]["accessToken"]|"")[0]&&!(auth["data"]["token"]|"")[0]){
    auth["token"]=response.cookies.token.c_str();reply="";serializeJson(auth,reply);
   }
  }
 }
 if(xtouch_cloud_challenge(status,reply.c_str(),response.mitigated.c_str(),response.ray.c_str())){
  challengeStatus=status;
  if(auth){authBlocked[origin]=true;authBlockedAt[origin]=millis();authRetryAfter=300;}
  code=-12;
 }
 return code;
}
String executeJob() {
 DynamicJsonDocument input(6144);deserializeJson(input,jobInput);wipe(jobInput);
 String mode=input["action"]|"",region=input["region"]|"cn",token=input["token"]|"";
 if(mode=="validate_lan")return XTouchLanProbe::run(input.as<JsonVariantConst>());
 // Output pools are allocated by each action after HTTPS releases its TLS buffers.
 // Do not hold an unused 4 KiB JSON pool during the request on memory-limited boards.
 DynamicJsonDocument out(0);
 String body,reply;int code=-1;
 String remembered;
#if XTOUCH_FEATURE_CLOUD_LOGIN
 if(mode=="login"||mode=="login_code"||mode=="login_tfa"||mode=="send_code")
  code=XTouchCloudLogin::execute(input,out,mode,region,token,remembered,body,reply,request,uidFromToken);
 else
#endif
 if(mode=="devices") {
  input.clear();input.shrinkToFit();
  const auto *endpoint=XTouchCloudEndpoints::get(region.c_str());
  code=request(String(endpoint->website)+"/api/v1/iot-service/api/user/bind",token,"",reply);
  // The official website proxy and API can have different edge protections.
  // Both routes use this account's region and token; never cross regions.
  if(code==-12||code==404)code=request(String(endpoint->api)+"/v1/iot-service/api/user/bind",token,"",reply);
  XTouchCloudMemory::output(out,min((size_t)16384,max((size_t)2048,reply.length()*2)));
  DynamicJsonDocument data(min((size_t)24576,max((size_t)2048,reply.length()*2)));
  if(code==200&&!deserializeJson(data,reply)) {
   JsonArray devices=out.createNestedArray("devices");unsigned count=0;
   for(JsonVariantConst device:data["devices"].as<JsonArrayConst>()) {
    if(count++>=100)break;JsonObject p=devices.createNestedObject();p["serialNumber"]=device["dev_id"];p["name"]=device["name"];p["printerModel"]=device["dev_product_name"];p["online"]=device["online"].as<bool>();
   }
   out["ok"]=true;
  }
 }
#if XTOUCH_FEATURE_CLOUD_TOKEN
 else {
  code=XTouchCloudToken::validate(input,out,region,token,reply,request);
 }
#endif
if(!out.capacity())XTouchCloudMemory::output(out);
#if XTOUCH_FEATURE_CLOUD_LOGIN
 if(remembered.length() && (out["ok"].as<bool>() || !strcmp(out["error"]|"","verification_required") || !strcmp(out["error"]|"","two_factor_required")))out["passwordEnc"]=remembered;
#endif
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
#if XTOUCH_FEATURE_CLOUD_LOGIN
  if(account["rememberPassword"].as<bool>()&&strlen(account["passwordEnc"]|"")){
   String password;if(!XTouchCredentials::read(account,false,password)){activeError="password_key_changed";return false;}
  }
#endif
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
 const char *action=r["action"]|"";
 const bool lan=!strcmp(action,"validate_lan");
 const bool tokenAction=!strcmp(action,"validate");
#if XTOUCH_FEATURE_CLOUD_LOGIN
 const bool accountAction=!strcmp(action,"login")||!strcmp(action,"login_code")||!strcmp(action,"send_code")||!strcmp(action,"login_tfa");
#else
 const bool accountAction=false;
#endif
 if(!lan&&!tokenAction&&!accountAction&&strcmp(action,"devices"))return false;
#if !XTOUCH_FEATURE_CLOUD_TOKEN
 if(tokenAction)return false;
#endif
 if(lan&&!XTouchSchema::lanCandidate(r))return false;
 const char *region=r["region"]|"cn";if(strcmp(region,"cn")&&strcmp(region,"global"))return false;
#if XTOUCH_FEATURE_CLOUD_LOGIN
 if(!strcmp(action,"login")&&(!r["account"].is<const char *>()||strlen(r["account"])>100||!r["password"].is<const char *>()||strlen(r["password"])>128))return false;
 if(accountAction&&(!r["account"].is<const char *>()||!strlen(r["account"])||strlen(r["account"])>100))return false;
 if(!strcmp(action,"login")&&!strlen(r["password"]|""))return false;
 if(!strcmp(action,"login_tfa")&&(!strlen(r["tfaKey"]|"")||strlen(r["tfaKey"]|"")>512))return false;
 if(!strcmp(action,"login_code")||!strcmp(action,"login_tfa")){const char *code=r["code"]|"";if(strlen(code)!=6)return false;for(const char *p=code;*p;++p)if(*p<'0'||*p>'9')return false;}
 if(!strcmp(action,"send_code")&&!strchr(r["account"]|"",'@')&&strcmp(region,"cn"))return false;
#endif
 if(!accountAction&&!lan&&(!r["token"].is<const char *>()||!strlen(r["token"])||strlen(r["token"])>2048))return false;
 wipe(jobOutput);serializeJson(r,jobInput);running=true;pending=true;
 // The main loop applies the board transport policy, then starts this deferred worker.
 return true;
}
String xtouch_cloud_job_result(){
 if(pending&&running) {
  pending=false;
  if(xTaskCreatePinnedToCore(job,"xt-cloud",XTouchNetworkBudgets::CloudWorkerStackBytes,nullptr,1,nullptr,0)!=pdPASS){running=false;wipe(jobInput);jobOutput="{\"ok\":false,\"error\":\"out_of_memory\"}";}
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
