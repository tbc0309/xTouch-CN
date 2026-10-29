#include "ble_provisioning.h"
#include "platform/idf/runtime.h"
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>


#define XTOUCH_BLE_MODEL "S3"

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
bool failedAttempt=false;
unsigned credentialRevision=0;
wifi_sta_config_t received={};

void events(void *,esp_event_base_t base,int32_t id,void *data) {
    if(base!=NETWORK_PROV_EVENT)return;
    portENTER_CRITICAL(&lock);
    if(id==NETWORK_PROV_WIFI_CRED_RECV && data){memcpy(&received,data,sizeof(received));++credentialRevision;}
    if(id==NETWORK_PROV_WIFI_CRED_SUCCESS)success=true;
    if(id==NETWORK_PROV_WIFI_CRED_FAIL){++state.failures;failedAttempt=true;}
    if(id==NETWORK_PROV_END)ended=true;
    if(id==NETWORK_PROV_DEINIT)deinitialized=true;
    portEXIT_CRITICAL(&lock);
}

void worker(void *) {
    bool initialized=false,stopping=false,deinitIssued=false;
    uint32_t deadline=0,retryAt=0;
    unsigned failedCandidates=0,seenCredentialRevision=0;
    for(;;) {
        int action; bool done,serviceEnded,deinitDone,failed;unsigned revision;
        portENTER_CRITICAL(&lock);
        action=request;
        if(action==1 && int32_t(millis()-startAt)<0)action=0;
        else request=0;
        done=stopRequested;serviceEnded=ended;deinitDone=deinitialized;failed=failedAttempt;failedAttempt=false;revision=credentialRevision;
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
            if(err==ESP_OK){err=network_prov_mgr_init(config);initialized=err==ESP_OK;}
            // Keep the Wi-Fi provisioning UUID used by the official mobile examples;
            // the newer network_provisioning component has a different default UUID.
            static uint8_t provisioningUuid[16]={
                0xb4,0xdf,0x5a,0x1c,0x3f,0x6b,0xf4,0xbf,
                0xea,0x4a,0x82,0x03,0x04,0x90,0x1a,0x02
            };
            if(err==ESP_OK)err=network_prov_scheme_ble_set_service_uuid(provisioningUuid);
            portENTER_CRITICAL(&lock);
            ended=stopRequested=deinitialized=failedAttempt=false;success=false;failedCandidates=0;credentialRevision=seenCredentialRevision=0;retryAt=0;memset(&received,0,sizeof(received));
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
        // Keep the failure visible long enough for the client to read it.
        // A session accepts at most three credential candidates.
        if(failed){++failedCandidates;retryAt=millis()+10000;}
        if(revision!=seenCredentialRevision){
            seenCredentialRevision=revision;
            if(failedCandidates>=3)done=true;
            else retryAt=0;
        }
        if(initialized&&active&&retryAt&&int32_t(millis()-retryAt)>=0){
            retryAt=0;
            if(failedCandidates>=3)done=true;
            else {
                network_prov_wifi_sta_state_t wifiState;
                // ATTEMPT_FAILED is an intermediate retry; DISCONNECTED is the
                // terminal failure for which the manager accepts a state reset.
                if(network_prov_mgr_get_wifi_state(&wifiState)==ESP_OK&&wifiState==NETWORK_PROV_WIFI_STA_DISCONNECTED){
                    esp_err_t reset=network_prov_mgr_reset_wifi_sm_state_on_failure();
                    if(reset!=ESP_OK){portENTER_CRITICAL(&lock);state.error=reset;portEXIT_CRITICAL(&lock);done=true;}
                }
            }
        }
        if(initialized && !stopping && (action==2 || done || serviceEnded || (active && int32_t(millis()-deadline)>=0))) {
            stopping=true;
            if(active) {
                // Stop first; deinitialize once after NETWORK_PROV_END.
                if(!serviceEnded)network_prov_mgr_stop_provisioning();
            } else {
                // Initialization succeeded but the service never started: no END callback follows.
                network_prov_mgr_deinit();deinitIssued=true;
            }
        }
        // The native manager owns teardown after its END event.
        if(initialized && stopping && serviceEnded && !deinitIssued) {
            network_prov_mgr_deinit();deinitIssued=true;
        }
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
    const bool allowed=available&&!state.finished && (start?(!state.active&&!state.starting):(state.active||state.starting));
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
