#include "ble_provisioning.h"
#include <Arduino.h>
#if CONFIG_IDF_TARGET_ESP32S3
// The SDK transport bypasses Arduino BLE classes; reserve controller memory
// before initArduino() would otherwise release it permanently.
#include <esp32-hal-alloc-ble-mem.h>
#endif
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include <esp_hosted.h>
#endif
#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

#ifndef XTOUCH_P4_C6_PREINITIALIZED_BT
#define XTOUCH_P4_C6_PREINITIALIZED_BT 0
#endif

#if CONFIG_IDF_TARGET_ESP32P4
#define XTOUCH_BLE_MODEL "P4"
#else
#define XTOUCH_BLE_MODEL "S3"
#endif

namespace {
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
struct Status {
    uint32_t starts=0,successes=0,failures=0,remaining=0;
    int error=0;
    bool active=false,starting=false,finished=false,credentials=false;
    char name[24]={},pop[17]={},ssid[33]={},password[65]={};
} state;
int request=0;
uint32_t startAt=0;
bool ended=false,stopRequested=false,deinitialized=false,success=false;
bool available=false;
wifi_sta_config_t received={};

void events(void *,esp_event_base_t base,int32_t id,void *data) {
    if(base!=NETWORK_PROV_EVENT)return;
    portENTER_CRITICAL(&lock);
    if(id==NETWORK_PROV_WIFI_CRED_RECV && data)memcpy(&received,data,sizeof(received));
    if(id==NETWORK_PROV_WIFI_CRED_SUCCESS)success=true;
    if(id==NETWORK_PROV_WIFI_CRED_FAIL){++state.failures;stopRequested=true;}
    if(id==NETWORK_PROV_END)ended=true;
    if(id==NETWORK_PROV_DEINIT)deinitialized=true;
    portEXIT_CRITICAL(&lock);
}

void worker(void *) {
    bool initialized=false,stopping=false,deinitIssued=false;
#if CONFIG_IDF_TARGET_ESP32P4
    bool controllerInitialized=false,controllerEnabled=false;
#endif
    uint32_t deadline=0;
    for(;;) {
        int action; bool done,serviceEnded,deinitDone;
        portENTER_CRITICAL(&lock);
        action=request;
        if(action==1 && int32_t(millis()-startAt)<0)action=0;
        else request=0;
        done=stopRequested;serviceEnded=ended;deinitDone=deinitialized;
        portEXIT_CRITICAL(&lock);
        if(action==2 && !initialized){
            portENTER_CRITICAL(&lock);state.starting=false;state.finished=true;memset(state.pop,0,sizeof(state.pop));portEXIT_CRITICAL(&lock);
        }
        if(action==1 && !initialized) {
            Serial.printf("[BLE] BLE init internal=%u largest=%u\n",unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)),unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)));
            network_prov_mgr_config_t config={};
            config.scheme=network_prov_scheme_ble;
            config.network_prov_wifi_conn_cfg.wifi_conn_attempts=3;
            esp_err_t err=ESP_OK;
#if CONFIG_IDF_TARGET_ESP32P4 && !XTOUCH_P4_C6_PREINITIALIZED_BT
            err=esp_hosted_bt_controller_init();controllerInitialized=err==ESP_OK;
            if(err==ESP_OK){err=esp_hosted_bt_controller_enable();controllerEnabled=err==ESP_OK;}
#endif
            if(err==ESP_OK){err=network_prov_mgr_init(config);initialized=err==ESP_OK;}
            // Keep the Wi-Fi provisioning UUID used by the official mobile examples;
            // the newer network_provisioning component has a different default UUID.
            static uint8_t provisioningUuid[16]={
                0xb4,0xdf,0x5a,0x1c,0x3f,0x6b,0xf4,0xbf,
                0xea,0x4a,0x82,0x03,0x04,0x90,0x1a,0x02
            };
            if(err==ESP_OK)err=network_prov_scheme_ble_set_service_uuid(provisioningUuid);
            portENTER_CRITICAL(&lock);
            ended=stopRequested=deinitialized=false;success=false;memset(&received,0,sizeof(received));
            portEXIT_CRITICAL(&lock);
            if(initialized && err==ESP_OK)err=network_prov_mgr_start_provisioning(NETWORK_PROV_SECURITY_1,state.pop,state.name,nullptr);
            portENTER_CRITICAL(&lock);
            state.error=err;state.starting=false;state.active=err==ESP_OK;
            if(err==ESP_OK)++state.starts;
            portEXIT_CRITICAL(&lock);
            if(err==ESP_OK)deadline=millis()+180000;
            else {done=true;Serial.printf("[BLE] BLE start failed: %s\n",esp_err_to_name(err));}
        }
        bool active;
        portENTER_CRITICAL(&lock);active=state.active;portEXIT_CRITICAL(&lock);
        if(initialized && !stopping && (action==2 || done || serviceEnded || (active && int32_t(millis()-deadline)>=0))) {
            stopping=true;
            if(active) {
                // Arduino WiFiGeneric owns deinit after NETWORK_PROV_END on S3.
                // Stop first; deinitializing concurrently with it deletes the same mutex twice.
                if(!serviceEnded)network_prov_mgr_stop_provisioning();
            } else {
                // Initialization succeeded but the service never started: no END callback follows.
                network_prov_mgr_deinit();deinitIssued=true;
            }
        }
#if CONFIG_ESP_HOSTED_ENABLED
        // Hosted Wi-Fi does not install Arduino's automatic provisioning cleanup.
        if(initialized && stopping && serviceEnded && !deinitIssued) {
            network_prov_mgr_deinit();deinitIssued=true;
        }
#endif
        if(initialized && stopping && deinitDone) {
            initialized=stopping=deinitIssued=false;
            esp_wifi_set_storage(WIFI_STORAGE_RAM);
            portENTER_CRITICAL(&lock);
            if(success) {
                memcpy(state.ssid,received.ssid,32);state.ssid[32]=0;
                memcpy(state.password,received.password,64);state.password[64]=0;
                state.credentials=true;++state.successes;
            }
            memset(&received,0,sizeof(received));memset(state.pop,0,sizeof(state.pop));
            state.active=state.starting=false;state.remaining=0;state.finished=true;ended=stopRequested=deinitialized=false;
            portEXIT_CRITICAL(&lock);
            Serial.printf("[BLE] BLE stopped; credentials verified=%d\n",success);
        } else if(done && !initialized) {
            portENTER_CRITICAL(&lock);state.finished=true;state.starting=false;ended=stopRequested=deinitialized=false;portEXIT_CRITICAL(&lock);
        }
        if(!initialized && !board_ble_busy()) {
#if CONFIG_IDF_TARGET_ESP32P4
            if(controllerEnabled){esp_hosted_bt_controller_disable();controllerEnabled=false;}
            if(controllerInitialized){esp_hosted_bt_controller_deinit(false);controllerInitialized=false;}
#endif
        }
        const uint32_t now=millis();
        portENTER_CRITICAL(&lock);
        if(state.active)state.remaining=int32_t(deadline-now)>0?(deadline-now+999)/1000:0;
        portEXIT_CRITICAL(&lock);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
}

void board_ble_begin() {
    // IDF Security 1 logs its derived session key at INFO; never expose it.
    // The provisioning manager can also print Wi-Fi passwords at DEBUG.
    esp_log_level_set("security1", ESP_LOG_WARN);
    esp_log_level_set("network_prov_mgr", ESP_LOG_WARN);
    esp_event_handler_register(NETWORK_PROV_EVENT,ESP_EVENT_ANY_ID,events,nullptr);
    available=xTaskCreate(worker,"ble-provision",6144,nullptr,2,nullptr)==pdPASS;
    if(!available)state.error=ESP_ERR_NO_MEM;
}
bool board_ble_request(bool start) {
    uint32_t random=esp_random();
    portENTER_CRITICAL(&lock);
    const bool allowed=available&&!state.finished && (!start || (!state.active&&!state.starting));
    if(allowed){
        request=start?1:2;
        if(start){
            state.starting=true;state.error=0;startAt=millis()+1500;
            // Official provisioning apps filter manual discovery by PROV_ by default.
            snprintf(state.name,sizeof(state.name),"PROV_XT" XTOUCH_BLE_MODEL "_%06lx",(unsigned long)(random&0xffffff));
            strlcpy(state.pop,"abcd1234",sizeof(state.pop));
        }
    }
    portEXIT_CRITICAL(&lock);return allowed;
}
bool board_ble_busy(){portENTER_CRITICAL(&lock);bool b=state.active||state.starting;portEXIT_CRITICAL(&lock);return b;}
bool board_ble_finished(char *ssid,size_t ssidSize,char *password,size_t passwordSize) {
    portENTER_CRITICAL(&lock);
    const bool ready=state.finished;
    if(ready) {
        strlcpy(ssid,state.credentials?state.ssid:"",ssidSize);strlcpy(password,state.credentials?state.password:"",passwordSize);
        memset(state.ssid,0,sizeof(state.ssid));memset(state.password,0,sizeof(state.password));state.finished=state.credentials=false;
    }
    portEXIT_CRITICAL(&lock);return ready;
}
void board_ble_status(JsonObject out) {
    portENTER_CRITICAL(&lock);auto s=state;portEXIT_CRITICAL(&lock);
    out["freeInternal"]=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    out["bleActive"]=s.active;out["bleStarting"]=s.starting;out["bleRemaining"]=s.remaining;
    out["bleStarts"]=s.starts;out["bleSuccesses"]=s.successes;out["bleFailures"]=s.failures;
    out["error"]=s.error;
    if(s.active||s.starting){out["serviceName"]=s.name;out["proofOfPossession"]=s.pop;}
}
