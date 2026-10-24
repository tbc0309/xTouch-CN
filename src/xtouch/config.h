#ifndef _XLCD_JSONCONFIG
#define _XLCD_JSONCONFIG

#include <ArduinoJson.h>
#include "ui/palette.h"
#include "filesystem.h"
#include "paths.h"
#include "printers.h"
#include "storage.h"
#include "profile_schema.h"
#include "cloud.h"
#include "credentials.h"
#include "portal.h"
#include "profile_route.h"

struct XTouchPrinterProfile {
    char name[64];
    char host[64];
    char accessCode[9];
    char serialNumber[32];
    char printerModel[32];
    char accountId[32];
    int8_t health;
    uint8_t source; // LAN, Token, China phone, international email.
    bool cloud;
    bool configured;
};
static XTouchPrinterProfile *xtouch_printers;
static uint8_t xtouch_profile_capacity;
static uint8_t xtouch_active_printer = 0;
static int xtouch_pending_printer = -1;
static bool xtouch_profiles_loaded = false;
static char xtouch_edited_serial[32]={},xtouch_edited_host[64]={};
void xtouch_printer_edit_hint(const char *serial,const char *host){
 strlcpy(xtouch_edited_serial,serial?serial:"",sizeof(xtouch_edited_serial));
 strlcpy(xtouch_edited_host,host?host:"",sizeof(xtouch_edited_host));
}


bool xtouch_printer_local_credentials(char *host,size_t hostSize,char *code,size_t codeSize){
 if(!host||!code||hostSize<2||codeSize<9||!xtouch_printers||!xtouch_profile_capacity)return false;
 host[0]=code[0]=0;const auto &active=xtouch_printers[xtouch_active_printer];
 if(!XTouchProfileRoute::local(active.configured,active.cloud)||!active.host[0]||strlen(active.accessCode)!=8)return false;
 strlcpy(host,active.host,hostSize);strlcpy(code,active.accessCode,codeSize);return true;
}
bool xtouch_printer_camera_credentials(char *host,size_t hostSize,char *code,size_t codeSize){
 if(!host||!code||hostSize<2||codeSize<9||!xtouch_printers||!xtouch_profile_capacity)return false;
 host[0]=code[0]=0;
 const auto *match=XTouchProfileRoute::cameraProfile(xtouch_printers,xtouch_profile_capacity,xtouch_active_printer);
 if(!match)return false;
 strlcpy(host,match->host,hostSize);strlcpy(code,match->accessCode,codeSize);return true;
}

bool xtouch_printer_cloud(void) { return xtouch_printers && xtouch_profile_capacity && xtouch_printers[xtouch_active_printer].cloud; }

uint8_t xtouch_printer_source(uint8_t slot) { return xtouch_printer_configured(slot)?xtouch_printers[slot].source:0; }
uint32_t xtouch_printer_source_color(uint8_t slot) { static const uint32_t colors[]={UI_COLOR_SOURCE_LAN,UI_COLOR_SOURCE_TOKEN,UI_COLOR_SOURCE_PHONE,UI_COLOR_SOURCE_EMAIL}; const uint8_t source=xtouch_printer_source(slot); return colors[source<4?source:0]; }

const char *xtouch_printer_account(void){return xtouch_printer_cloud()?xtouch_printers[xtouch_active_printer].accountId:"";}
int xtouch_printer_cloud_health(uint8_t slot){return xtouch_printer_configured(slot)?xtouch_printers[slot].health:0;}
void xtouch_printer_set_cloud_health(const char *account,int state,const char *serials){
 for(unsigned i=0;i<xtouch_profile_capacity;++i){auto &p=xtouch_printers[i];if(!p.cloud||strcmp(p.accountId,account))continue;
  String needle=String("|")+p.serialNumber+"|";p.health=state==1&&serials&&!strstr(serials,needle.c_str())?-1:state;
 }
}
bool xtouch_printer_configured(uint8_t slot) {
    return xtouch_printers && slot < xtouch_profile_capacity && xtouch_printers[slot].configured;
}
uint8_t xtouch_printer_active(void) { return xtouch_active_printer; }
const char *xtouch_printer_name(void) {
    return xtouch_printer_configured(xtouch_active_printer)
        ? xtouch_printers[xtouch_active_printer].name : "--";
}
const char *xtouch_printer_name_at(uint8_t slot) {
    return xtouch_printer_configured(slot) ? xtouch_printers[slot].name : "--";
}
const char *xtouch_printer_model_at(uint8_t slot) {
    if (!xtouch_printer_configured(slot)) return "";
    if (slot==xtouch_active_printer && bambuStatus.printer_type[0]) return bambuStatus.printer_type;
    const auto &profile=xtouch_printers[slot];
    if(profile.printerModel[0])return profile.printerModel;
    return "";
}
bool xtouch_printer_switch_pending(void) { return xtouch_pending_printer >= 0; }
bool xtouch_printer_request(uint8_t slot) {
    if (!xtouch_printer_configured(slot)) return false;
    if(xtouch_printers[slot].cloud&&xtouch_printers[slot].health<0){xtouch_portal_show_account(xtouch_printers[slot].accountId);return false;}
    if(slot==xtouch_active_printer)return false;
    xtouch_pending_printer = slot; // Process after LVGL returns, never delete a clicked widget here.
    return true;
}

static void xtouch_printer_apply(uint8_t slot) {
    const auto &p = xtouch_printers[slot];
    xtouch_active_printer = slot;
    xtouch_cloud_select(p.cloud ? p.accountId : "");
    strlcpy(xTouchConfig.xTouchAccessCode, p.accessCode, sizeof(xTouchConfig.xTouchAccessCode));
    strlcpy(xTouchConfig.xTouchSerialNumber, p.serialNumber, sizeof(xTouchConfig.xTouchSerialNumber));
    strlcpy(xTouchConfig.xTouchHost, p.host, sizeof(xTouchConfig.xTouchHost));
    strlcpy(xTouchConfig.xTouchPrinterModel, p.printerModel, sizeof(xTouchConfig.xTouchPrinterModel));
    strlcpy(xTouchConfig.xTouchPrinterName, p.name, sizeof(xTouchConfig.xTouchPrinterName));
}

static void xtouch_printer_parse(JsonVariantConst value, uint8_t slot) {
    if (slot>=xtouch_profile_capacity || !value.is<JsonObjectConst>()) return;
    auto &p = xtouch_printers[slot];
    const char *host = value["host"] | "";
    String code;
    const char *serial = value["serialNumber"] | "";
    const char *model = value["printerModel"] | "";
    const char *name = value["name"] | "";
    if (!XTouchSchema::printer(value)) return;
    p.cloud = !strcmp(value["mode"] | "lan", "cloud");
    if(!p.cloud&&!XTouchCredentials::readAccessCode(value,code))return;
    strlcpy(p.accountId, value["accountId"] | "", sizeof(p.accountId));
    strlcpy(p.host, host, sizeof(p.host));
    strlcpy(p.accessCode, code.c_str(), sizeof(p.accessCode));
    strlcpy(p.serialNumber, serial, sizeof(p.serialNumber));
    strlcpy(p.printerModel, model, sizeof(p.printerModel));
    if (name[0]&&XTouchSchema::asciiName(value["name"])) strlcpy(p.name, name, sizeof(p.name));
    else snprintf(p.name, sizeof(p.name), "Printer %u", unsigned(slot + 1));
    p.configured = true;
}


DynamicJsonDocument xtouch_load_config()
{
    DynamicJsonDocument config = xtouch_storage_read(xtouch_paths_config, 0);
    if (!config.isNull() && !xtouch_profiles_loaded) {
        if(!xtouch_printers){
            xtouch_profile_capacity=config["printers"].is<JsonArray>()?min((size_t)XTOUCH_MAX_PRINTERS,config["printers"].size()):1;
            xtouch_printers=(XTouchPrinterProfile*)calloc(max(1,(int)xtouch_profile_capacity),sizeof(XTouchPrinterProfile));
        }
        if(!xtouch_printers){ConsoleError.println("[PRINTER] profile allocation failed");return config;}
        if (config["printers"].is<JsonArray>()) {
            JsonArray list = config["printers"].as<JsonArray>();
            for (uint8_t i = 0; i < XTOUCH_MAX_PRINTERS && i < list.size(); i++)
                xtouch_printer_parse(list[i], i);
            for(uint8_t i=0;i<xtouch_profile_capacity;++i) {
                auto &p=xtouch_printers[i]; p.source=p.cloud?1:0;
                if(p.cloud)for(JsonObject a:config["cloudAccounts"].as<JsonArray>())
                    if(!strcmp(a["id"]|"",p.accountId)&&!strcmp(a["loginMethod"]| (a["authMode"]|"token"),"password"))p.source=!strcmp(a["region"]|"cn","global")?3:2;
            }
        }
        unsigned requested=config["activePrinter"] | 0U;
        uint8_t selected=requested<XTOUCH_MAX_PRINTERS?requested:XTOUCH_MAX_PRINTERS;
        if (!xtouch_printer_configured(selected)) {
            for (selected = 0; selected < XTOUCH_MAX_PRINTERS; selected++)
                if (xtouch_printer_configured(selected)) break;
        }
        if (selected < XTOUCH_MAX_PRINTERS) xtouch_printer_apply(selected);
        xtouch_profiles_loaded = true;
    }

    return config;
}

// Rebuild the saved list without changing the selected printer to the boot default.
static bool xtouch_profiles_reload(bool &reconnect) {
    DynamicJsonDocument config=xtouch_storage_read(xtouch_paths_config,0);
    if(config.isNull())return false;
    const unsigned count=min((size_t)XTOUCH_MAX_PRINTERS,config["printers"].size());
    auto *replacement=(XTouchPrinterProfile*)calloc(count+1,sizeof(XTouchPrinterProfile));
    if(!replacement)return false;
    XTouchPrinterProfile previous={};
    const bool hadPrinter=xtouch_printer_configured(xtouch_active_printer);
    if(hadPrinter)previous=xtouch_printers[xtouch_active_printer];
    String oldToken=xtouch_cloud_token(),oldBroker=xtouch_cloud_broker();
    const unsigned oldPort=xtouch_cloud_port();
    auto *old=xtouch_printers;
    xtouch_printers=replacement;xtouch_profile_capacity=count+1;
    unsigned selected=count; // Empty sentinel: deleting the selected device disconnects it.
    for(unsigned i=0;i<count;++i){
        xtouch_printer_parse(config["printers"][i],i);
        auto &profile=xtouch_printers[i];profile.source=profile.cloud?1:0;
        if(profile.cloud)for(JsonObject a:config["cloudAccounts"].as<JsonArray>())
            if(!strcmp(a["id"]|"",profile.accountId)&&!strcmp(a["loginMethod"]|(a["authMode"]|"token"),"password"))profile.source=!strcmp(a["region"]|"cn","global")?3:2;
    }
    if(hadPrinter){int match=XTouchProfileRoute::reloadedSelection(xtouch_printers,count,previous);if(match>=0)selected=unsigned(match);}
    if(hadPrinter&&!previous.cloud&&selected==count&&xtouch_edited_serial[0]){
        for(unsigned i=0;i<count;++i){const auto &p=xtouch_printers[i];
            if(p.configured&&!p.cloud&&!strcmp(p.serialNumber,xtouch_edited_serial)&&!strcmp(p.host,xtouch_edited_host)){selected=i;break;}
        }
    }
    xtouch_printer_edit_hint("","");
    if(!hadPrinter&&count)selected=min(config["activePrinter"]|0U,count-1);
    xtouch_active_printer=selected;xtouch_pending_printer=-1;free(old);
    const auto &current=xtouch_printers[selected];
    if(current.configured)xtouch_printer_apply(selected);
    else {xtouch_cloud_select("");xTouchConfig.xTouchHost[0]=0;xTouchConfig.xTouchAccessCode[0]=0;xTouchConfig.xTouchSerialNumber[0]=0;xTouchConfig.xTouchPrinterName[0]=0;xTouchConfig.xTouchPrinterModel[0]=0;}
    reconnect=hadPrinter!=current.configured||previous.cloud!=current.cloud||
        strcmp(previous.host,current.host)||strcmp(previous.accessCode,current.accessCode)||
        strcmp(previous.serialNumber,current.serialNumber)||strcmp(previous.accountId,current.accountId)||
        oldToken!=xtouch_cloud_token()||oldBroker!=xtouch_cloud_broker()||oldPort!=xtouch_cloud_port();
    if(!reconnect&&hadPrinter)xtouch_printers[selected].health=previous.health;
    for(unsigned i=0;i<oldToken.length();++i)oldToken.setCharAt(i,0);
    return true;
}

#endif
