#include "features/config.h"
#include "clock.h"
#include "features/power/controller.h"
#include "features/network/wifi_recovery.h"
#if XTOUCH_FEATURE_BLE_PROVISIONING
#include "ble_provisioning.h"
#endif
#include "features/sound/events.h"
#include "features/capabilities.h"
#include "features/settings_validation.h"
#include "features/preview/print_preview.h"
#include "power.h"
#include "types.h"
#ifdef XTOUCH_WAVESHARE
#include "camera.h"
#endif
#include <esp_heap_caps.h>
#include "portal.h"
#include "storage.h"
#include "profile_schema.h"
#include "cloud.h"
#include "vault.h"
#include "credentials.h"
#include <Update.h>
#include "firmware_target.h"
#include "ui/settings_gesture.h"
#include "printers.h"
#include "ui/i18n/language.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <XTouchWebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include <esp_system.h>
#include <esp_idf_version.h>
#if ESP_IDF_VERSION_MAJOR >= 6
#include <psa/crypto.h>
#else
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#endif
#include "portal_assets.h"
#include "ui/layout_shared.h"
#if defined(XTOUCH_UI_NATIVE_CANVAS) && XTOUCH_UI_NATIVE_CANVAS
#include "display_scale.h"
#endif

extern "C" { LV_FONT_DECLARE(ui_font_portal_title_22); void ui_confirmPanel_show(const char*,void(*)(void)); void ui_confirmPanel_show_danger(const char*,void(*)(void)); void loadScreen(int); extern lv_obj_t *introScreen; extern lv_obj_t *introScreenCaption; extern lv_obj_t *introScreenIcon; }
extern void xtouch_portal_release_transport();
namespace {
XTouchWebServer server(80);DNSServer dns;
String session,csrf,adminSalt,adminHash,apSsid;
bool mustChange=true,apMode=false,qrActive=false;
int showRequest=0;uint32_t rebootAt=0,sessionTouched=0,lockedUntil=0,cloudNext=0,codeNext=0,connectAt=0,scanAt=0;unsigned failures=0;
bool manualAP=false,bootComplete=false;
#if XTOUCH_FEATURE_BLE_PROVISIONING
bool bleScreen=false;
#endif
uint32_t wifiApplyAt=0,apExitAt=0;
String scanCache="[]";bool scanning=false,scanFailed=false;
uint32_t scanStartedAt=0;
// Both board SDKs expose the shared scan deadline as a protected static member.
// AP+STA scans can exceed Arduino 2.x's six-second default while keeping the AP alive.
class PortalScanDeadline:public WiFiScanClass{public:static void extend(){_scanTimeout=30000;}};

bool uploading=false,uploadOk=false,uploadDenied=false;unsigned uploadFiles=0;String uploadError,accountTarget;

struct AccountHealth{String id;int state=0;bool renewed=false;};
AccountHealth health[XTouchSchema::MaxAccounts];unsigned healthCount=0,healthIndex=0;bool background=false,renewing=false;uint32_t checkAt=20000,nextSweep=21600000;String manualAccount;
void inspectResult(JsonVariantConst result,const String &id){
 if(result["ok"].as<bool>()&&result["devices"].is<JsonArrayConst>()){
  String serials;for(JsonObjectConst p:result["devices"].as<JsonArrayConst>())serials+=String("|")+(p["serialNumber"]|"")+"|";
  xtouch_printer_set_cloud_health(id.c_str(),1,serials.c_str());
 }else if(!strcmp(result["error"]|"","authentication_rejected"))xtouch_printer_set_cloud_health(id.c_str(),-1,nullptr);
}
String binding(JsonVariantConst a){return XTouchCredentials::binding(a);}
String randomHex(unsigned bytes=16) {String out;out.reserve(bytes*2);for(unsigned i=0;i<bytes;++i){char b[3];snprintf(b,sizeof(b),"%02x",(uint8_t)esp_random());out+=b;}return out;}
String passwordHash(const String &password,const String &salt) {
 uint8_t result[32];
#if ESP_IDF_VERSION_MAJOR >= 6
 psa_key_derivation_operation_t operation=PSA_KEY_DERIVATION_OPERATION_INIT;
 int rc=psa_crypto_init();
 if(!rc)rc=psa_key_derivation_setup(&operation,PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
 if(!rc)rc=psa_key_derivation_input_integer(&operation,PSA_KEY_DERIVATION_INPUT_COST,10000);
 if(!rc)rc=psa_key_derivation_input_bytes(&operation,PSA_KEY_DERIVATION_INPUT_SALT,(const uint8_t*)salt.c_str(),salt.length());
 if(!rc)rc=psa_key_derivation_input_bytes(&operation,PSA_KEY_DERIVATION_INPUT_PASSWORD,(const uint8_t*)password.c_str(),password.length());
 if(!rc)rc=psa_key_derivation_output_bytes(&operation,result,sizeof(result));
 psa_key_derivation_abort(&operation);
#else
 mbedtls_md_context_t context;mbedtls_md_init(&context);
 if(mbedtls_md_setup(&context,mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),1))return "";
 int rc=mbedtls_pkcs5_pbkdf2_hmac(&context,(const unsigned char *)password.c_str(),password.length(),(const unsigned char *)salt.c_str(),salt.length(),10000,sizeof(result),result);
 mbedtls_md_free(&context);
#endif
 if(rc)return "";String out;out.reserve(64);for(auto value:result){char b[3];snprintf(b,sizeof(b),"%02x",value);out+=b;}return out;
}
bool equal(const String &a,const String &b){if(a.length()!=b.length())return false;uint8_t diff=0;for(size_t i=0;i<a.length();++i)diff|=a[i]^b[i];return !diff;}
bool saveAdmin(){StaticJsonDocument<256>doc;doc["salt"]=adminSalt;doc["hash"]=adminHash;doc["mustChange"]=mustChange;File f=SPIFFS.open("/xtouch/admin.json.tmp",FILE_WRITE);if(!f)return false;size_t n=serializeJson(doc,f);f.flush();f.close();if(n!=measureJson(doc))return false;SPIFFS.remove("/xtouch/admin.json.previous");if(SPIFFS.exists("/xtouch/admin.json"))SPIFFS.rename("/xtouch/admin.json","/xtouch/admin.json.previous");if(!SPIFFS.rename("/xtouch/admin.json.tmp","/xtouch/admin.json")){SPIFFS.rename("/xtouch/admin.json.previous","/xtouch/admin.json");return false;}return true;}
void loadAdmin(){StaticJsonDocument<256>doc;File f=SPIFFS.open("/xtouch/admin.json");if(f&&deserializeJson(doc,f)==DeserializationError::Ok){adminSalt=doc["salt"]|"";adminHash=doc["hash"]|"";mustChange=doc["mustChange"]|true;}f.close();if(adminSalt.length()!=32||adminHash.length()!=64){adminSalt=randomHex();adminHash=passwordHash("12345678",adminSalt);mustChange=true;saveAdmin();}}
void headers(){server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'");server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Content-Type-Options","nosniff");server.sendHeader("X-Frame-Options","DENY");server.sendHeader("Referrer-Policy","no-referrer");}
void json(int code,const String &body){headers();server.send(code,"application/json; charset=utf-8",body);}
void error(int code,const char *name){json(code,String("{\"error\":\"")+name+"\"}");}
bool origin(){String value=server.header("Origin");return value.isEmpty()||value=="http://"+server.hostHeader();}
bool hasSession(){
 String cookie=server.header("Cookie"),needle="xtouch_session="+session;bool present=false;
 int start=0;while(start<cookie.length()){int end=cookie.indexOf(';',start);if(end<0)end=cookie.length();String part=cookie.substring(start,end);part.trim();if(equal(part,needle))present=true;start=end+1;}
 return !session.isEmpty()&&present&&millis()-sessionTouched<=1800000;
}
// Provisioning without a session is limited to the device hotspot interface.
bool setupAccess(){return apMode&&server.client().localIP()==WiFi.softAPIP();}
bool authenticated(bool changeAllowed=false){
 if(!hasSession()){error(401,"login_required");return false;}
 if(!origin()){error(403,"origin");return false;}
 if(server.method()!=HTTP_GET&&!equal(server.header("X-XTouch-CSRF"),csrf)){error(403,"csrf");return false;}
 sessionTouched=millis();if(mustChange&&!changeAllowed){error(403,"password_change_required");return false;}return true;
}
bool body(JsonDocument &doc,size_t limit=32768){
 if(!origin()){error(403,"origin");return false;}
 if(!server.hasArg("plain")){error(400,"invalid_json");return false;}
 const String &input=server.argReference("plain");
 if(input.length()>limit){error(400,"invalid_json");return false;}
 auto result=deserializeJson(doc,input);
 if(result==DeserializationError::NoMemory||doc.overflowed()){error(503,"low_memory");return false;}
 if(result){error(400,"invalid_json");return false;}return true;
}
class LabBody:public Stream{public:String value;bool overflow=false;size_t write(uint8_t c)override{if(value.length()>=16384){overflow=true;return 0;}value+=(char)c;return 1;}int available()override{return 0;}int read()override{return -1;}int peek()override{return -1;}void flush()override{}};
void labDiscover(){
 if(!authenticated())return;
 if(xtouch_cloud_busy()){error(409,"cloud_busy");return;}
 StaticJsonDocument<256> request;if(!body(request))return;
 const char *host=request["host"]|"";unsigned port=request["port"]|8894U;
 if(!XTouchSchema::labHost(host)||!port||port>65535){error(400,"invalid_configuration");return;}
 xtouch_portal_release_transport();WiFiClient client;HTTPClient http;http.setConnectTimeout(1500);http.setTimeout(2000);http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
 if(!http.begin(client,String("http://")+host+":"+port+"/api/discovery")){error(502,"cloud_network_error");return;}
 int code=http.GET();LabBody sink;sink.value.reserve(16384);if(code==200)http.writeToStream(&sink);http.end();
 if(code!=200||sink.overflow){error(502,"cloud_network_error");return;}
 DynamicJsonDocument data(24576);if(deserializeJson(data,sink.value)||!data["devices"].is<JsonArray>()||data["devices"].size()>100){error(400,"invalid_configuration");return;}
 for(JsonVariantConst p:data["devices"].as<JsonArrayConst>())if(!XTouchSchema::text(p["serialNumber"],32,true)||strncmp(p["serialNumber"]|"","XTSIM",5)||!XTouchSchema::asciiName(p["name"])||!XTouchSchema::text(p["printerModel"],32,true)){error(400,"invalid_configuration");return;}
 json(200,sink.value);
}
void login(){
 if((int32_t)(millis()-lockedUntil)<0){server.sendHeader("Retry-After","30");error(429,"login_throttled");return;}
 DynamicJsonDocument doc(512);if(!body(doc,256))return;
 String password=doc["password"]|"";if(password.length()>128||!equal(passwordHash(password,adminSalt),adminHash)){
  if(++failures>=5){lockedUntil=millis()+30000;failures=0;}error(401,"invalid_password");return;
 }
 failures=0;bool same=hasSession();
 if(!same){session=randomHex();csrf=randomHex();}sessionTouched=millis();server.sendHeader("Set-Cookie","xtouch_session="+session+"; Path=/; HttpOnly; SameSite=Strict");
 json(200,String("{\"csrf\":\"")+csrf+"\",\"mustChange\":"+(mustChange?"true":"false")+"}");
}
void changePassword(){if(!authenticated(true))return;DynamicJsonDocument doc(512);if(!body(doc,384))return;String password=doc["password"]|"";
 if(password.length()<8||password.length()>64||password=="12345678"){error(400,"password_length");return;}
 String oldHash=adminHash,oldSalt=adminSalt;bool oldChange=mustChange;adminSalt=randomHex();adminHash=passwordHash(password,adminSalt);mustChange=false;
 if(!saveAdmin()){adminHash=oldHash;adminSalt=oldSalt;mustChange=oldChange;error(507,"storage_failed");return;}session="";csrf="";server.sendHeader("Set-Cookie","xtouch_session=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict");json(200,"{\"ok\":true,\"loginRequired\":true}");
}
void configGet(){if(!authenticated())return;if(xtouch_cloud_busy()){error(409,"cloud_busy");return;}
 // Only memory-constrained targets trade their printer TLS session for config RAM.
 // Reading settings on PSRAM boards must not force another blocking handshake.
#if XTOUCH_PORTAL_RECLAIM_TLS_ON_READ
 xtouch_portal_release_transport();
#endif
 DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);
 const size_t capacity=max(size_t(4096),saved.capacity()+saved["printers"].size()*64+saved["cloudAccounts"].size()*192+4096);
 DynamicJsonDocument doc(capacity);if(!doc.set(saved)||doc.overflowed()){error(503,"low_memory");return;}if(doc.isNull()){doc.to<JsonObject>();doc["ssid"]="";doc["activePrinter"]=0;doc.createNestedArray("printers");}
 doc["wifiPasswordSaved"]=strlen(doc["pwd"]|"")>0;doc["pwd"]="";
 unsigned slot=0;for(JsonObject p:doc["printers"].as<JsonArray>()){
  p["health"]=xtouch_printer_cloud_health(slot++);String plain;
  p["accessCodeEncrypted"]=XTouchCredentials::accessCodeEncrypted(p);
  p["accessCodeSaved"]=XTouchCredentials::readAccessCode(p,plain);
  p["accessCodeLocked"]=strlen(p["accessCodeEnc"]|"")&&!p["accessCodeSaved"].as<bool>();
  p["accessCode"]="";p.remove("accessCodeEnc");
 }
 for(JsonObject a:doc["cloudAccounts"].as<JsonArray>()){a["tokenEncrypted"]=XTouchCredentials::encrypted(a,true);a["passwordEncrypted"]=XTouchCredentials::encrypted(a,false);String plain;a["tokenSaved"]=XTouchCredentials::read(a,true,plain);a["credentialLocked"]=strlen(a["tokenEnc"]|"")&&!a["tokenSaved"].as<bool>();a["token"]="";a.remove("tokenEnc");a["passwordSaved"]=XTouchCredentials::read(a,false,plain);a.remove("password");a["passwordLocked"]=strlen(a["passwordEnc"]|"")&&!a["passwordSaved"].as<bool>();if(a["passwordLocked"].as<bool>())a["tokenSaved"]=false;a.remove("passwordEnc");for(unsigned i=0;i<healthCount;++i)if(health[i].id==(a["id"]|""))a["health"]=health[i].state<0?"invalid":health[i].state>0?"valid":"unknown";}
 DynamicJsonDocument settings=xtouch_storage_read("/xtouch/settings.json",0);doc["settings"]=settings;String out;serializeJson(doc,out);json(200,out);
}
bool mergeSecrets(DynamicJsonDocument &doc, DynamicJsonDocument &old){
 if(old.isNull()&&SPIFFS.exists("/xtouch.json"))return false;
 if(!strlen(doc["pwd"]|""))doc["pwd"]=old["pwd"].as<String>();
 for(JsonObject p:doc["printers"].as<JsonArray>()) {
  if(!strcmp(p["mode"]|"lan","cloud"))continue;
  JsonVariantConst previous;
  JsonVariantConst source=p["credentialSource"].is<JsonObject>()?p["credentialSource"].as<JsonVariantConst>():JsonVariantConst(p);
  for(JsonVariantConst oldP:old["printers"].as<JsonArrayConst>())if(!strcmp(oldP["mode"]|"lan","lan")&&!strcmp(source["serialNumber"]|"",oldP["serialNumber"]|"")&&!strcmp(source["host"]|"",oldP["host"]|"")){previous=oldP;break;}
  p.remove("credentialSource");
  if(!XTouchCredentials::mergeAccessCode(p,previous))return false;
 }
  for(JsonObject a:doc["cloudAccounts"].as<JsonArray>()) {
  if(a["simulator"].as<bool>()){a["token"]="xtouch-lab";a.remove("passwordSaved");a.remove("passwordLocked");a.remove("credentialLocked");continue;}
  JsonVariantConst previous;for(JsonObject oldA:old["cloudAccounts"].as<JsonArray>())if(binding(a)==binding(oldA)||
   (a["credentialSource"].is<JsonObject>()&&!strcmp(a["id"]|"",oldA["id"]|"")&&!strcmp(a["credentialSource"]["region"]|"",oldA["region"]|"cn")&&!strcmp(a["credentialSource"]["account"]|"",oldA["account"]|""))){previous=oldA;break;}
  a.remove("credentialSource");
  if(!XTouchCredentials::merge(a,previous,true))return false;
  if(a["rememberPassword"].as<bool>()){if(!XTouchCredentials::merge(a,previous,false))return false;}else {a.remove("password");a.remove("passwordEnc");}
  a.remove("credentialLocked");a.remove("passwordSaved");a.remove("passwordLocked");
  for(const char *key:{"health","expanded","devices","needsCode","verified","lastError","tfaKey"})a.remove(key);
 }
 return !doc.overflowed();
}
void configSave(){if(!authenticated())return;if(xtouch_cloud_busy()||background){error(409,"cloud_busy");return;}
#if XTOUCH_FEATURE_BLE_PROVISIONING
 if(board_ble_busy()){error(409,"busy");return;}
#endif
#if XTOUCH_PORTAL_RECLAIM_TLS_ON_READ
 xtouch_portal_release_transport();
#endif
 DynamicJsonDocument previous=xtouch_storage_read("/xtouch.json",0);
 if(previous.isNull()&&SPIFFS.exists("/xtouch.json")){error(503,"low_memory");return;}
 const size_t inputBytes=server.argReference("plain").length();
 const size_t capacity=min(XTouchSchema::MaxConfigCapacity,max(size_t(4096),inputBytes*2+previous.memoryUsage()+2048));
 // Parsing, secret merging and atomic storage temporarily hold multiple copies.
 // Reject before exhausting heap; a valid configuration must remain usable.
 if(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)<capacity || heap_caps_get_free_size(MALLOC_CAP_8BIT)<capacity*3+inputBytes+8192){error(503,"low_memory");return;}
 DynamicJsonDocument doc(capacity);if(!body(doc,XTouchSchema::MaxConfigBytes))return;for(JsonVariantConst p:doc["printers"].as<JsonArrayConst>())if(!XTouchSchema::asciiName(p["name"])){error(400,"invalid_name");return;}for(JsonVariantConst a:doc["cloudAccounts"].as<JsonArrayConst>())if(!XTouchSchema::asciiName(a["name"])){error(400,"invalid_name");return;}if(!mergeSecrets(doc,previous)||!XTouchSchema::root(doc)){error(400,"invalid_configuration");return;}
 const bool wifiChanged=strcmp(previous["ssid"]|"",doc["ssid"]|"")||strcmp(previous["pwd"]|"",doc["pwd"]|"");
 DynamicJsonDocument settings(2048);settings.set(doc["settings"]);
 if(settings.overflowed()||!xtouch_feature_settings_valid(settings.as<JsonVariantConst>())){error(400,"invalid_settings");return;}
 doc.remove("settings");doc.remove("wifiPasswordSaved");
 for(JsonObject p:doc["printers"].as<JsonArray>()){p.remove("accessCodeSaved");p.remove("accessCodeLocked");p.remove("health");}for(JsonObject a:doc["cloudAccounts"].as<JsonArray>()){a.remove("tokenSaved");a.remove("clearToken");a.remove("health");}
 // Compare persisted fields only, so UI metadata never triggers a profile reload.
 const bool profilesChanged=previous["printers"]!=doc["printers"] || previous["cloudAccounts"]!=doc["cloudAccounts"];
 const bool rootChanged=previous.as<JsonVariantConst>()!=doc.as<JsonVariantConst>();
 int8_t savedHealthState[XTouchSchema::MaxAccounts];bool savedHealthRenewed[XTouchSchema::MaxAccounts];
 for(unsigned i=0;i<healthCount;++i){savedHealthState[i]=health[i].state;savedHealthRenewed[i]=health[i].renewed;}
 // A label-only edit must not invalidate a previously verified account.
 if(profilesChanged)for(unsigned i=0;i<healthCount;++i){
  JsonObject before,after;
  for(JsonObject a:previous["cloudAccounts"].as<JsonArray>())if(health[i].id==(a["id"]|""))before=a;
  for(JsonObject a:doc["cloudAccounts"].as<JsonArray>())if(health[i].id==(a["id"]|""))after=a;
  if(after["loginCleared"].as<bool>())savedHealthState[i]=-1;
  else if(before.isNull()||after.isNull()||binding(before)!=binding(after)||before["token"]!=after["token"]||before["tokenEnc"]!=after["tokenEnc"]||before["password"]!=after["password"]||before["passwordEnc"]!=after["passwordEnc"]){savedHealthState[i]=0;savedHealthRenewed[i]=false;}
 }
 previous.clear();previous.shrinkToFit();
 DynamicJsonDocument oldSettings=xtouch_storage_read("/xtouch/settings.json",0);
 const bool settingsChanged=oldSettings.as<JsonVariantConst>()!=settings.as<JsonVariantConst>();
 oldSettings.clear();oldSettings.shrinkToFit();
 if(rootChanged&&!xtouch_storage_write("/xtouch.json",doc)){error(507,"storage_failed");return;}
 if(settingsChanged&&!settings.isNull()&&!xtouch_storage_write("/xtouch/settings.json",settings)){error(507,"settings_failed");return;}
 if(profilesChanged){
  // Compact in place to keep String-bearing account entries off the callback stack.
  unsigned kept=0;
  for(unsigned i=0;i<healthCount;++i){
   bool exists=false;
   for(JsonObject a:doc["cloudAccounts"].as<JsonArray>())if(!a["simulator"].as<bool>()&&health[i].id==(a["id"]|"")){exists=true;break;}
   if(exists){if(kept!=i)health[kept]=health[i];health[kept].state=savedHealthState[i];health[kept].renewed=savedHealthRenewed[i];++kept;}
  }
  const unsigned previousCount=healthCount;
  for(JsonObject a:doc["cloudAccounts"].as<JsonArray>())if(!a["simulator"].as<bool>()){
   bool exists=false;for(unsigned i=0;i<kept;++i)if(health[i].id==(a["id"]|"")){exists=true;break;}
   if(!exists&&kept<XTouchSchema::MaxAccounts){auto &entry=health[kept++];entry=AccountHealth{};entry.id=a["id"].as<String>();if(a["loginCleared"].as<bool>())entry.state=-1;}
  }
  for(unsigned i=kept;i<previousCount;++i)health[i]=AccountHealth{};
  healthCount=kept;healthIndex=0;
  checkAt=millis()+60000;
 }
 if(profilesChanged||settingsChanged)xtouch_runtime_config_reload(profilesChanged);
 if(wifiChanged)wifiApplyAt=millis()+500;
 json(200,String("{\"ok\":true,\"restarting\":false,\"reconnecting\":")+(wifiChanged?"true":"false")+"}");
}
void cancelScan(){
 if(scanning){esp_wifi_scan_stop();WiFi.scanDelete();scanning=false;scanFailed=false;scanAt=0;}
}
void scan(){if(!origin()){error(403,"origin");return;}if(!setupAccess()&&!authenticated())return;
#if XTOUCH_FEATURE_BLE_PROVISIONING
 if(board_ble_busy()){error(409,"busy");return;}
#endif
 const bool refresh=server.arg("refresh")=="1";
 if(scanFailed&&!refresh){error(503,"wifi_scan_failed");return;}
 if(!scanning&&(refresh||!scanAt||millis()-scanAt>30000)){
  scanStartedAt=millis();
  const int result=WiFi.scanNetworks(true,true);
  if(result==WIFI_SCAN_RUNNING)PortalScanDeadline::extend();
  Serial.printf("[WIFI-SCAN] start=%d mode=%d status=%d\n",result,int(WiFi.getMode()),int(WiFi.status()));
  scanFailed=result==WIFI_SCAN_FAILED;
  if(scanFailed){scanAt=0;error(503,"wifi_scan_failed");return;}
  scanning=true;
 }
 json(200,String("{\"scanning\":")+(scanning?"true":"false")+",\"networks\":"+scanCache+"}");
}
int saveWifiCredentials(const char *ssid,const char *password){
 auto previous=xtouch_storage_read("/xtouch.json",0);
 if(previous.isNull()&&SPIFFS.exists("/xtouch.json"))return 503;
 DynamicJsonDocument config(max(size_t(1024),previous.capacity()+512));
 if(!previous.isNull()&&!config.set(previous))return 503;
 config["ssid"]=ssid;config["pwd"]=password;
 if(!config["printers"].is<JsonArray>())config.createNestedArray("printers");
 config["activePrinter"]=config["activePrinter"]|0;
 if(config.overflowed())return 503;
 return xtouch_storage_write("/xtouch.json",config)?0:507;
}
bool saveSetupLanguage(const char *language,bool initializeScreen=true){
 if(xtouch_language_index(language)<0)return false;
 auto previous=xtouch_storage_read("/xtouch/settings.json",0);
 if(previous.isNull()&&SPIFFS.exists("/xtouch/settings.json"))return false;
 DynamicJsonDocument settings(max(size_t(1024),previous.capacity()+128));
 if(!previous.isNull()&&!settings.set(previous))return false;
 if(initializeScreen)settings["language"]=language;
 settings["portalLanguage"]=language;
 if(settings.overflowed()||!xtouch_storage_write("/xtouch/settings.json",settings))return false;
 strlcpy(xTouchConfig.xTouchPortalLanguage,language,sizeof(xTouchConfig.xTouchPortalLanguage));
 if(initializeScreen)xtouch_runtime_config_reload(false);
 return true;
}
void wifiSave(){if(!setupAccess()&&!authenticated())return;DynamicJsonDocument input(512);if(!body(input,384))return;
#if XTOUCH_FEATURE_BLE_PROVISIONING
 if(board_ble_busy()){error(409,"busy");return;}
#endif
 if(input.containsKey("language")&&(!input["language"].is<const char*>()||xtouch_language_index(input["language"])<0)){error(400,"invalid_settings");return;}
 if(!XTouchSchema::text(input["ssid"],33,true)||!XTouchSchema::text(input["pwd"],65)){error(400,"invalid_wifi");return;}
 int failure=saveWifiCredentials(input["ssid"],input["pwd"]|"");
 if(failure){error(failure,failure==503?"low_memory":"storage_failed");return;}
 if(input.containsKey("language")&&!saveSetupLanguage(input["language"])){error(507,"settings_failed");return;}
 json(200,"{\"ok\":true,\"reconnecting\":true}");wifiApplyAt=millis()+500;
}
void cloudStart(){if(!authenticated())return;if(background){error(409,"checking_accounts");return;}checkAt=millis()+60000;if(server.arg("plain").indexOf("\"devices\"")<0&&(int32_t)(millis()-cloudNext)<0){server.sendHeader("Retry-After",String((cloudNext-millis()+999)/1000));error(429,"wait_before_retry");return;}xtouch_portal_release_transport();
 DynamicJsonDocument request(6144);if(!body(request,5500))return;
 if(!strcmp(request["action"]|"","login")&&!strlen(request["password"]|"")){
  DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);
  for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(!strcmp(a["id"]|"",request["accountId"]|"")&&!strcmp(a["account"]|"",request["account"]|"")&&!strcmp(a["region"]|"cn",request["region"]|"cn")){
   String password;if(XTouchCredentials::read(a,false,password))request["password"]=password;
  }
 }
 bool sendingCode=!strcmp(request["action"]|"","send_code");if(sendingCode&&(int32_t)(millis()-codeNext)<0){error(429,"wait_before_retry");return;}
 if(!strlen(request["token"]|"")&&request["accountId"].is<const char *>()) {DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(!strcmp(a["id"]|"",request["accountId"])&&!strcmp(a["region"]|"cn",request["credentialSource"]["region"]| (request["region"]|"cn"))){String plain;if(!XTouchCredentials::read(a,true,plain)&&strlen(a["tokenEnc"]|"")){error(409,"credential_key_changed");return;}request["token"]=plain;}}
 request.remove("credentialSource");
 request.remove("credentialSource");
 if(!xtouch_cloud_job_start(request.as<JsonVariantConst>())){error(400,"invalid_or_busy");return;}
 manualAccount=request["accountId"].as<String>();cloudNext=millis()+5000;if(sendingCode)codeNext=millis()+60000;json(202,"{\"busy\":true}");
}
void validateLanStart(){
 if(!authenticated())return;
 if(background||xtouch_cloud_busy()||uploading||rebootAt){error(409,"busy");return;}
#ifdef XTOUCH_WAVESHARE
 if(xtouch_camera_status().open){error(409,"busy");return;}
#endif
 StaticJsonDocument<768> request;if(!body(request,512))return;
 request["action"]="validate_lan";request["mode"]="lan";request["region"]="cn";
 if(!strlen(request["accessCode"]|"")){
  DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);
  JsonVariantConst source=request["credentialSource"].is<JsonObject>()?request["credentialSource"].as<JsonVariantConst>():request.as<JsonVariantConst>();
  for(JsonVariantConst p:saved["printers"].as<JsonArrayConst>())if(!strcmp(p["mode"]|"lan","lan")&&!strcmp(p["serialNumber"]|"",source["serialNumber"]|"")&&!strcmp(p["host"]|"",source["host"]|"")){String plain;if(XTouchCredentials::readAccessCode(p,plain))request["accessCode"]=plain;break;}
  request.remove("credentialSource");
 }
 if(!XTouchSchema::lanCandidate(request.as<JsonVariantConst>())){error(400,"invalid_configuration");return;}
 xtouch_portal_release_transport();checkAt=millis()+60000;
 if(!xtouch_cloud_job_start(request.as<JsonVariantConst>())){error(409,"busy");return;}
 json(202,"{\"busy\":true}");
}
// Internal reset preserves the SD source. A fresh key makes old encrypted passwords unreadable.
void factoryReset(){
 if(xtouch_cloud_busy()||uploading)return;
 if(!xtouch_vault_rotate()){Serial.println("[RESET] key rotation failed");return;}
 const char *paths[]={"/xtouch.json","/xtouch/settings.json","/xtouch/touch.json","/xtouch/hardware.json","/xtouch/admin.json"};
 for(const char *path:paths)for(const char *suffix:{"",".tmp",".previous"})SPIFFS.remove(String(path)+suffix);
 rebootAt=millis()+500;
}
void firmwareChunk(){
 // WebServer also invokes this callback for raw POST bodies, without HTTPUpload.
 if(!server.header("Content-Type").startsWith("multipart/form-data")){uploadOk=false;uploadError="application_firmware_required";return;}
 HTTPUpload &upload=server.upload();
 if(upload.status==UPLOAD_FILE_START){
  if(++uploadFiles>1){uploadError="one_firmware_required";Update.abort();return;}uploadOk=false;uploadError="";
  if(!authenticated()){uploadDenied=true;uploadError="login_required";return;}if(xtouch_cloud_busy()||rebootAt){uploadError="not_ready";return;}
  xtouch_portal_release_transport();uploading=true;
 }else if(upload.status==UPLOAD_FILE_WRITE && uploading && uploadError.isEmpty()){
  if(!Update.isRunning()){if(!xtouch_firmware_header_valid(upload.buf,upload.currentSize)){uploadError="application_firmware_required";return;}if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)){uploadError="update_begin_failed";return;}}
  if(upload.totalSize+upload.currentSize>XTOUCH_FIRMWARE_MAX_BYTES||Update.write(upload.buf,upload.currentSize)!=upload.currentSize){uploadError="firmware_write_failed";Update.abort();}
 }else if(upload.status==UPLOAD_FILE_END){
  if(uploading&&uploadError.isEmpty())uploadOk=true;
  if(!uploadOk&&uploadError.isEmpty())uploadError="invalid_firmware";
  if(!uploadOk)Update.abort();uploading=false;
 }else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();uploading=false;uploadOk=false;uploadDenied=false;uploadFiles=0;uploadError="upload_aborted";}
}
const char *setupText(const char *zh,const char *en){return bootComplete?xtouch_local(zh,en):en;}
void setupFrame(const char *text){
 loadScreen(-1);qrActive=true;
 lv_obj_add_flag(introScreenIcon,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(introScreenCaption,LV_OBJ_FLAG_HIDDEN);
 lv_obj_set_layout(introScreen,0);lv_obj_set_style_pad_all(introScreen,0,0);
 lv_obj_set_style_bg_color(introScreen,lv_color_hex(0x151515),0);
 lv_obj_t *title=lv_label_create(introScreen);lv_obj_set_style_text_font(title,&ui_font_portal_title_22,0);
 lv_obj_set_style_text_color(title,lv_color_white(),0);lv_obj_set_width(title,304);
 lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(title,LV_LABEL_LONG_CLIP);
 lv_label_set_text(title,text);lv_obj_align(title,LV_ALIGN_TOP_MID,0,6);
}
lv_obj_t *setupButton(const char *text,int y,int request,bool management=false){
 lv_obj_t *button=lv_btn_create(introScreen);lv_obj_set_size(button,240,28);lv_obj_align(button,LV_ALIGN_TOP_MID,0,y);
 ui_layout_setup_button(button,management);lv_obj_set_style_pad_ver(button,0,0);
 lv_obj_t *label=lv_label_create(button);lv_label_set_text(label,text);lv_obj_set_width(label,LV_PCT(100));
 lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
 lv_obj_add_event_cb(label,ui_layout_single_line_event,LV_EVENT_SIZE_CHANGED,nullptr);lv_obj_update_layout(button);ui_layout_single_line(label);lv_obj_center(label);
 lv_obj_add_event_cb(button,[](lv_event_t *event){showRequest=(int)(intptr_t)lv_event_get_user_data(event);},LV_EVENT_CLICKED,(void*)(intptr_t)request);
 return button;
}
void setupCaption(const char *text,int y){
 lv_obj_t *label=lv_label_create(introScreen);lv_obj_set_width(label,304);lv_obj_set_style_text_color(label,lv_color_hex(0xC2D0D8),0);
 lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);
 lv_label_set_text(label,text);lv_obj_align(label,LV_ALIGN_TOP_MID,0,y);
}
lv_obj_t *setupQr(const String &value){
 lv_obj_t *code=lv_qrcode_create(introScreen,128,lv_color_hex(0x151515),lv_color_white());lv_obj_align(code,LV_ALIGN_TOP_MID,0,38);
 lv_qrcode_update(code,value.c_str(),value.length());return code;
}
lv_obj_t *setupReturn(lv_obj_t *code,int request,bool management=false){
 if(!bootComplete)return nullptr;
 lv_obj_t *back=setupButton(xtouch_local("返回设置","Back to settings"),206,request,management);
 ui_layout_match_width(back,code);return back;
}
void qr(bool wifi) {
 setupFrame(wifi?setupText("扫码连接 Wi-Fi","Scan to join Wi-Fi"):xtouch_local("扫码管理","Scan to manage"));
 String ip=WiFi.localIP().toString();
 String value=wifi?"WIFI:T:WPA;S:"+apSsid+";P:12345678;;":"http://"+ip+"/"+(accountTarget.length()?"#account="+accountTarget:"");
 lv_obj_t *code=setupQr(value),*back=setupReturn(code,3,!wifi);
 ui_layout_qr_details(code,back,wifi?apSsid.c_str():ip.c_str(),wifi?"192.168.4.1 / 12345678":nullptr,!wifi);
}
#if XTOUCH_FEATURE_BLE_PROVISIONING
void setupChooser(){
 setupFrame("Network setup");bleScreen=false;
 setupCaption("Choose a connection method",39);
 for(int method=0;method<2;++method){
  lv_obj_t *card=setupButton(method?"Bluetooth":"Wi-Fi",72+method*66,method?5:2);
  lv_obj_set_size(card,288,58);lv_obj_set_style_radius(card,8,0);
  lv_obj_set_style_bg_color(card,lv_color_hex(0x252525),0);
  lv_obj_t *label=lv_obj_get_child(card,0);lv_obj_set_width(label,220);
  lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
  lv_obj_align(label,LV_ALIGN_TOP_LEFT,44,5);
  lv_obj_t *slot=lv_obj_create(card);lv_obj_remove_style_all(slot);lv_obj_set_size(slot,32,32);
  lv_obj_clear_flag(slot,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_align(slot,LV_ALIGN_LEFT_MID,-2,0);
  lv_obj_t *icon=lv_label_create(slot);lv_obj_set_style_text_font(icon,&lv_font_montserrat_28,0);
  lv_obj_set_style_text_color(icon,lv_color_hex(0x2AFF00),0);
  lv_label_set_text(icon,method?LV_SYMBOL_BLUETOOTH:LV_SYMBOL_WIFI);
  lv_obj_add_event_cb(slot,ui_layout_setup_symbol_event,LV_EVENT_SIZE_CHANGED,nullptr);
  lv_obj_update_layout(card);lv_event_send(slot,LV_EVENT_SIZE_CHANGED,nullptr);
  lv_obj_t *detail=lv_label_create(card);lv_label_set_text(detail,method?"ESP BLE Provisioning app":"Phone or computer browser");
  lv_obj_set_style_text_color(detail,lv_color_hex(0xAAAAAA),0);lv_obj_align(detail,LV_ALIGN_BOTTOM_LEFT,44,-3);
 }
 setupCaption("xTouch-CN",215);
}
void bluetoothQr(){
 StaticJsonDocument<1536> status;board_ble_status(status.to<JsonObject>());
 setupFrame(setupText("扫码蓝牙配网","Scan to join Bluetooth"));bleScreen=true;
 StaticJsonDocument<192> payload;payload["ver"]="v1";payload["name"]=status["serviceName"];payload["pop"]="abcd1234";payload["transport"]="ble";
 String value;serializeJson(payload,value);lv_obj_t *code=setupQr(value);
 lv_obj_t *back=setupReturn(code,6);
 ui_layout_qr_details(code,back,status["serviceName"]|"","ESP BLE Prov / abcd1234",false);
}
#endif

void startAP(){apExitAt=0;if(apMode)return;apMode=true;WiFi.mode(WIFI_AP_STA);apSsid="XTOUCH-CN-"+String((uint32_t)ESP.getEfuseMac(),HEX);WiFi.softAP(apSsid.c_str(),"12345678");dns.start(53,"*",WiFi.softAPIP());qr(true);}
}
extern "C" void xtouch_portal_show_management(){accountTarget="";showRequest=1;}
extern "C" void xtouch_portal_show_account(const char *id){accountTarget=id;showRequest=1;}
extern "C" void xtouch_portal_request_wifi(){showRequest=2;}
extern "C" void xtouch_portal_request_bluetooth(){showRequest=5;}
void xtouch_portal_boot_complete(){bootComplete=true;}
extern "C" void xtouch_portal_request_reset(){showRequest=4;}
extern "C" bool xtouch_portal_screen_active(){return qrActive;}
bool xtouch_portal_ap(){return apMode;}
extern "C" bool xtouch_portal_provisioning(){
#if XTOUCH_FEATURE_BLE_PROVISIONING
 return board_ble_busy();
#else
 return false;
#endif
}
bool xtouch_portal_maintenance(){return rebootAt||uploading||xtouch_cloud_busy()
#if XTOUCH_FEATURE_BLE_PROVISIONING
 ||board_ble_busy()
#endif
 ;}
void xtouch_portal_cloud_rejected(){
 const char *id=xtouch_printer_account();for(unsigned i=0;i<healthCount;++i)if(health[i].id==id&&!health[i].renewed&&!background){healthIndex=i;checkAt=millis()+5000;break;}
}
void xtouch_portal_begin(){
 WiFi.persistent(false);WiFi.mode(WIFI_STA);
 WiFi.setAutoReconnect(true);
#if XTOUCH_FEATURE_BLE_PROVISIONING
 board_ble_begin();

#endif
 Serial.printf("[VAULT] authenticated encryption tests: %s\n",xtouch_vault_selftest()?"PASS":"FAIL");loadAdmin();const char *keys[]={"Cookie","Origin","X-XTouch-CSRF","Content-Type"};server.collectHeaders(keys,4);
 server.on("/",HTTP_GET,[]{headers();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/html; charset=utf-8",(const char*)portal_index,sizeof(portal_index));});
 server.on("/portal.js",HTTP_GET,[]{headers();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/javascript; charset=utf-8",(const char*)portal_script,sizeof(portal_script));});
 server.on("/portal.css",HTTP_GET,[]{headers();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/css; charset=utf-8",(const char*)portal_style,sizeof(portal_style));});
 server.on("/api/info",HTTP_GET,[]{DynamicJsonDocument d(4096+healthCount*64);d["ap"]=setupAccess();d["portalLanguage"]=xTouchConfig.xTouchPortalLanguage[0]?xTouchConfig.xTouchPortalLanguage:"en";if(hasSession()&&!mustChange){JsonObject accountHealth=d.createNestedObject("accountHealth");for(unsigned i=0;i<healthCount;++i)accountHealth[health[i].id]=health[i].state;d["ip"]=WiFi.localIP().toString();d["ssid"]=WiFi.SSID();lv_mem_monitor_t graphics;lv_mem_monitor(&graphics);d["lvglFree"]=graphics.free_size;d["lvglLargest"]=graphics.free_biggest_size;xtouch_feature_diagnostics(d);d["cpuMHz"]=ESP.getCpuFreqMHz();auto power=xtouch_power_status();auto pm=d.createNestedObject("power");pm["screenSleeping"]=power.screenSleeping;pm["automaticDfs"]=power.automaticDfs;pm["automaticLightSleep"]=power.automaticLightSleep;pm["minMHz"]=power.minMHz;pm["maxMHz"]=power.maxMHz;pm["error"]=power.error;pm["wakeCause"]=power.wakeCause;pm["deepSleepAfter"]=xTouchConfig.xTouchDeepSleepAfter;d["heap"]=ESP.getFreeHeap();d["minHeap"]=ESP.getMinFreeHeap();d["heap8"]=heap_caps_get_free_size(MALLOC_CAP_8BIT);d["largestHeap8"]=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);d["mode"]=xtouch_cloud_simulator()?"Simulator":xtouch_printer_cloud()?"Cloud":"LAN";d["clockReady"]=time(nullptr)>1577836800;char clockText[6];d["clockLocal"]=xtouch_clock_text(clockText,sizeof(clockText))?clockText:"";d["timezone"]=xTouchConfig.xTouchTimezone;d["ntpServer"]=xTouchConfig.xTouchNtpServer;d["uptime"]=millis()/1000;
#if XTOUCH_FEATURE_BLE_PROVISIONING
 auto radio=d.createNestedObject("radio");board_ble_status(radio);radio.remove("proofOfPossession");radio.remove("serviceName");
#endif
d["online"]=xtouch_printer_online();d["printer"]=xtouch_printer_name();d["version"]=XTOUCH_FIRMWARE_VERSION;d["resetReason"]=(int)esp_reset_reason();auto preview=xtouch_print_preview_status();JsonObject pv=d.createNestedObject("preview");pv["busy"]=preview.busy;pv["visible"]=preview.visible;pv["width"]=preview.width;pv["height"]=preview.height;pv["attempts"]=preview.attempts;pv["result"]=preview.result;pv["stage"]=preview.stage;pv["httpStatus"]=preview.httpStatus;pv["tlsError"]=preview.tlsError;pv["renderedLines"]=preview.renderedLines;pv["tlsPhase"]=preview.tlsPhase;pv["printState"]=(int)bambuStatus.print_status;pv["hasTaskId"]=bambuStatus.task_id[0]&&strcmp(bambuStatus.task_id,"0");pv["hasSubtaskId"]=bambuStatus.obj_subtask_id[0]&&strcmp(bambuStatus.obj_subtask_id,"0");pv["hasFile"]=bool(bambuStatus.gcode_file[0]||bambuStatus.subtask_name[0]);
#ifdef XTOUCH_WAVESHARE
 auto cs=xtouch_camera_status();auto cam=d.createNestedObject("camera");cam["open"]=cs.open;cam["running"]=cs.running;cam["state"]=cs.state;cam["frames"]=cs.frames;cam["connections"]=cs.connections;cam["width"]=cs.width;cam["height"]=cs.height;cam["lastFrameAgeMs"]=cs.lastFrameAgeMs;cam["freeInternal"]=cs.freeInternal;cam["largestInternal"]=cs.largestInternal;
#endif
d["clockEpoch"]=(uint32_t)time(nullptr);}if(d.overflowed()){error(503,"low_memory");return;}String out;serializeJson(d,out);json(200,out);});
 server.on("/api/language",HTTP_POST,[]{
  if(!authenticated(true))return;
  StaticJsonDocument<96> input;if(!body(input,96))return;
  const char *value=input["language"]|"";
  if(xtouch_language_index(value)<0){error(400,"invalid_settings");return;}
  if(!saveSetupLanguage(value,false)){error(507,"settings_failed");return;}
  json(200,"{\"ok\":true}");
 });
 server.on("/api/login",HTTP_POST,login);
 server.on("/api/password",HTTP_POST,changePassword);
 server.on("/api/logout",HTTP_POST,[]{if(!authenticated(true))return;session="";csrf="";server.sendHeader("Set-Cookie","xtouch_session=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict");xtouch_cloud_job_clear();json(200,"{\"ok\":true}");});
 server.on("/api/session",HTTP_GET,[]{if(!authenticated(true))return;json(200,String("{\"csrf\":\"")+csrf+"\",\"mustChange\":"+(mustChange?"true":"false")+"}");});
 server.on("/api/reset",HTTP_POST,[]{if(!authenticated())return;StaticJsonDocument<64> d;if(!body(d,64))return;if(strcmp(d["confirm"]|"","RESET")){error(400,"confirmation_required");return;}if(xtouch_cloud_busy()||uploading){error(409,"busy");return;}json(200,"{\"ok\":true}");factoryReset();});
 server.setUploadGuard([](){return authenticated();});
 server.on("/api/firmware",HTTP_POST,[]{if(uploadDenied){uploadDenied=false;uploadFiles=0;return;}if(!authenticated()){Update.abort();uploadFiles=0;return;}uploadFiles=0;if(uploadOk&&uploadError.isEmpty())uploadOk=Update.end(true);if(uploadOk&&uploadError.isEmpty()){json(200,"{\"ok\":true}");rebootAt=millis()+1800;}else {Update.abort();error(400,uploadError.isEmpty()?"update_failed":uploadError.c_str());}},firmwareChunk);
 server.on("/api/config",HTTP_GET,configGet);server.on("/api/config",HTTP_POST,configSave);
 server.on("/api/printer/validate",HTTP_POST,validateLanStart);
 server.on("/api/printer/validate",HTTP_GET,[]{if(!authenticated())return;if(background){error(409,"checking_accounts");return;}json(200,xtouch_cloud_job_result());});
 server.on("/api/wifi/scan",HTTP_GET,scan);server.on("/api/wifi",HTTP_POST,wifiSave);
 server.on("/api/display/power",HTTP_POST,[]{if(!authenticated())return;DynamicJsonDocument d(192);if(!body(d))return;const char *mode=d["mode"]|"";unsigned value=!strcmp(mode,"standby")?1:!strcmp(mode,"wake")?2:!strcmp(mode,"deep")?3:0;if(!xtouch_display_request(value)){error(409,"power_unavailable");return;}json(202,"{\"ok\":true}");});
 server.on("/api/sound/test",HTTP_POST,[]{if(!authenticated())return;DynamicJsonDocument d(128);if(!body(d))return;const char *kind=d["kind"]|"warning";if(strcmp(kind,"touch")&&strcmp(kind,"warning")&&strcmp(kind,"error")&&strcmp(kind,"complete")){error(400,"invalid_config");return;}XTouchSoundEvent event=!strcmp(kind,"touch")?XTOUCH_SOUND_TOUCH:!strcmp(kind,"error")?XTOUCH_SOUND_ERROR:!strcmp(kind,"complete")?XTOUCH_SOUND_COMPLETE:XTOUCH_SOUND_WARNING;if(!xtouch_sound_test(event)){error(409,"sound_unavailable");return;}json(202,"{\"ok\":true}");});
 server.on("/api/backup",HTTP_GET,[]{if(!authenticated())return;DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);DynamicJsonDocument doc(saved.capacity()+2048);doc.set(saved);doc["settings"]=xtouch_storage_read("/xtouch/settings.json",0);if(doc.overflowed()){error(503,"low_memory");return;}String out;serializeJson(doc,out);server.sendHeader("Content-Disposition","attachment; filename=xtouch.json");json(200,out);});
 server.on("/api/lab/discover",HTTP_POST,labDiscover);
 server.on("/api/cloud",HTTP_POST,cloudStart);server.on("/api/cloud",HTTP_GET,[]{if(!authenticated())return;if(background){json(200,"{\"busy\":true}");return;}
 String value=xtouch_cloud_job_result();if(!xtouch_cloud_busy()){
  DynamicJsonDocument d(min((size_t)18432,max((size_t)2048,value.length()*2)));deserializeJson(d,value);inspectResult(d,manualAccount);
  if(!strcmp(d["error"]|"","cloud_blocked"))cloudNext=millis()+60000;
  for(unsigned i=0;i<healthCount;++i)if(health[i].id==manualAccount){if(d["ok"].as<bool>())health[i].state=1;else if(!strcmp(d["error"]|"","authentication_rejected"))health[i].state=-1;}
 }json(200,value);});
 server.onNotFound([]{if(apMode){server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","");}else error(404,"not_found");});server.begin();
 DynamicJsonDocument config=xtouch_storage_read("/xtouch.json",0);for(JsonObject a:config["cloudAccounts"].as<JsonArray>())if(!a["simulator"].as<bool>()&&healthCount<XTouchSchema::MaxAccounts)health[healthCount++].id=a["id"].as<String>();String ssid=config["ssid"]|"",password=config["pwd"]|"";
 if(ssid.isEmpty()){
#if XTOUCH_FEATURE_BLE_PROVISIONING
 setupChooser();
#else
 startAP();
#endif
 }else{WiFi.begin(ssid.c_str(),password.c_str());connectAt=millis();}
}
void xtouch_portal_loop(){
#if XTOUCH_FEATURE_BLE_PROVISIONING
 char radioSsid[33]={},radioPassword[65]={};
 if(showRequest==6){
  showRequest=0;board_ble_request(false);
  // Navigation does not wait for BLE transport teardown; busy state still serializes radio work.
  if(bleScreen){bleScreen=false;qrActive=false;loadScreen(4);}
 }
 if(board_ble_finished(radioSsid,sizeof(radioSsid),radioPassword,sizeof(radioPassword))){
  bool saved=false;
  if(radioSsid[0]){
   saved=saveWifiCredentials(radioSsid,radioPassword)==0;
   if(saved&&!bootComplete)saved=saveSetupLanguage("en");
   Serial.printf("[RADIO] verified configuration saved=%d\n",saved);
  }
  memset(radioPassword,0,sizeof(radioPassword));
  WiFi.setAutoReconnect(true);
  {auto restored=xtouch_storage_read("/xtouch.json",0);wifiApplyAt=strlen(restored["ssid"]|"")?millis()+500:0;}
  if(bleScreen){bleScreen=false;qrActive=false;if(!radioSsid[0]){if(bootComplete)loadScreen(4);else setupChooser();}else loadScreen(bootComplete?0:-1);}
 }
 if(board_ble_busy()){
  // Let the SDK provisioning manager own Wi-Fi until it completes or times out.
  WiFi.setAutoReconnect(false);server.handleClient();return;
 }
#endif
 if((int32_t)(millis()-nextSweep)>=0&&!background){healthIndex=0;nextSweep=millis()+21600000;for(unsigned i=0;i<healthCount;++i)health[i].renewed=false;}

 if(background && !xtouch_cloud_busy() && !ui_scroll_busy(750)){
  String result=xtouch_cloud_job_result();DynamicJsonDocument d(min((size_t)18432,max((size_t)2048,result.length()*2)));deserializeJson(d,result);
  auto &h=health[healthIndex];
  if(renewing&&d["ok"].as<bool>()&&strlen(d["token"]|"")){
   DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(h.id==(a["id"]|"")){if(!XTouchCredentials::store(a,true,d["token"].as<String>())){d["ok"]=false;break;}a["userId"]=d["userId"];}
   if(d["ok"].as<bool>()&&xtouch_storage_write("/xtouch.json",root)){h.state=0;if(h.id==xtouch_printer_account())xtouch_cloud_select(h.id.c_str());}else h.state=-1;
   renewing=false;background=false;checkAt=millis()+1000;xtouch_cloud_job_clear();
  }else if(!renewing&&!strcmp(d["error"]|"","authentication_rejected")&&!h.renewed){
   h.renewed=true;DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);StaticJsonDocument<1536> req;String password;
   for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(h.id==(a["id"]|"")&&a["rememberPassword"].as<bool>()&&XTouchCredentials::read(a,false,password)){
    req["action"]="login";req["accountId"]=h.id;req["region"]=a["region"];req["account"]=a["account"];req["password"]=password;
   }
   xtouch_cloud_job_clear();if(!req.isNull()&&xtouch_cloud_job_start(req.as<JsonVariantConst>()))renewing=true;
   else {h.state=-1;inspectResult(d,h.id);background=false;++healthIndex;}
  }else{
   h.state=d["ok"].as<bool>()?1:!strcmp(d["error"]|"","authentication_rejected")||renewing?-1:0;
   if(renewing)xtouch_printer_set_cloud_health(h.id.c_str(),-1,nullptr);else inspectResult(d,h.id);
   renewing=false;background=false;++healthIndex;checkAt=millis()+2000;xtouch_cloud_job_clear();
  }
 }
#ifdef XTOUCH_WAVESHARE
 const bool cameraPriority=xtouch_camera_status().open;
#else
 const bool cameraPriority=false;
#endif
 if(!cameraPriority && !background && !ui_scroll_busy(3000) && healthIndex<healthCount && WiFi.status()==WL_CONNECTED && time(nullptr)>1577836800 && !xtouch_cloud_busy() && !uploading && !rebootAt && (int32_t)(millis()-checkAt)>=0){
  DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);StaticJsonDocument<3072> req;
  for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(health[healthIndex].id==(a["id"]|"")){
   if(a["loginCleared"].as<bool>()){health[healthIndex].state=-1;xtouch_printer_set_cloud_health(health[healthIndex].id.c_str(),-1,nullptr);break;}
   String token;bool tokenReady=XTouchCredentials::read(a,true,token);
   if(!tokenReady&&strlen(a["tokenEnc"]|"")){health[healthIndex].state=-1;xtouch_printer_set_cloud_health(health[healthIndex].id.c_str(),-1,nullptr);break;}
   req["action"]="devices";req["region"]=a["region"];req["token"]=token;
   String password;bool remembered=a["rememberPassword"].as<bool>()&&(strlen(a["passwordEnc"]|"")||strlen(a["password"]|""));
   if(!tokenReady&&!remembered){req.clear();break;}
   if(remembered&&!XTouchCredentials::read(a,false,password)){req.clear();health[healthIndex].state=-1;xtouch_printer_set_cloud_health(health[healthIndex].id.c_str(),-1,nullptr);}
   else if(!tokenReady&&remembered){req["action"]="login";req["accountId"]=a["id"];req["account"]=a["account"];req["password"]=password;renewing=true;health[healthIndex].renewed=true;}
  }
  if(!req["action"].isNull()){xtouch_portal_release_transport();background=xtouch_cloud_job_start(req.as<JsonVariantConst>());}if(!background){renewing=false;++healthIndex;}
 }

 server.handleClient();if(apMode)dns.processNextRequest();
 if(apExitAt&&(int32_t)(millis()-apExitAt)>=0){
  apExitAt=0;
  if(apMode&&manualAP){cancelScan();dns.stop();WiFi.softAPdisconnect(false);WiFi.mode(WIFI_STA);apMode=manualAP=false;Serial.println("[PORTAL] setup AP closed; station retained");}
 }
 if(wifiApplyAt&&(int32_t)(millis()-wifiApplyAt)>=0){
  wifiApplyAt=0;xtouch_portal_release_transport();
  DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);String ssid=saved["ssid"]|"",password=saved["pwd"]|"";
  WiFi.disconnect();if(ssid.isEmpty())startAP();else{WiFi.begin(ssid.c_str(),password.c_str());connectAt=millis();}
 }
 if(scanning){
  const int count=WiFi.scanComplete();
  if(count>=0){
   DynamicJsonDocument doc(8192);JsonArray list=doc.to<JsonArray>();
   for(int i=0;i<count&&list.size()<30;++i){if(WiFi.SSID(i).isEmpty())continue;JsonObject n=list.createNestedObject();n["ssid"]=WiFi.SSID(i);n["rssi"]=WiFi.RSSI(i);n["open"]=WiFi.encryptionType(i)==WIFI_AUTH_OPEN;}
   scanFailed=doc.capacity()==0||doc.overflowed();
   if(!scanFailed){scanCache="";serializeJson(doc,scanCache);}
   Serial.printf("[WIFI-SCAN] complete=%d visible=%u failed=%d elapsed=%lu\n",count,unsigned(list.size()),scanFailed,(unsigned long)(millis()-scanStartedAt));
   WiFi.scanDelete();scanning=false;scanAt=scanFailed?0:millis();
  }else if(count==WIFI_SCAN_FAILED){Serial.printf("[WIFI-SCAN] failed elapsed=%lu\n",(unsigned long)(millis()-scanStartedAt));WiFi.scanDelete();scanning=false;scanFailed=true;scanAt=0;}
 }

 if(WiFi.status()==WL_CONNECTED){
  connectAt=0;
  if(apMode&&!manualAP){
   dns.stop();WiFi.softAPdisconnect(true);apMode=false;
   if(qrActive){qrActive=false;loadScreen(bootComplete?0:-1);}
  }
 }else if(xtouch_wifi_fallback_due(millis(),connectAt,false)){connectAt=0;startAP();}
 int request=showRequest;showRequest=0;
#if XTOUCH_FEATURE_BLE_PROVISIONING
 if(request==5){
  if(!uploading&&!xtouch_cloud_busy()&&!wifiApplyAt){
   xtouch_portal_release_transport();cancelScan();if(apMode){dns.stop();WiFi.softAPdisconnect(false);WiFi.mode(WIFI_STA);apMode=false;}apExitAt=0;manualAP=false;connectAt=0;
   if(board_ble_request(true))bluetoothQr();
  }
  return;
 }
#endif
if(request==1){if(WiFi.status()==WL_CONNECTED)qr(false);else{manualAP=bootComplete;startAP();}}else if(request==2){manualAP=bootComplete;startAP();qr(true);}else if(request==3){qrActive=false;loadScreen(4);if(apMode&&manualAP)apExitAt=millis()+100;}else if(request==4){ui_confirmPanel_show_danger(xtouch_local("重置设备？保留SD备份", "Reset device? SD backup remains."),factoryReset);}
 if(rebootAt&&(int32_t)(millis()-rebootAt)>=0)ESP.restart();
}
