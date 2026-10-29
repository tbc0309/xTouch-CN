#include "wifi.h"
#include "features/network/wifi_credentials.h"
#include <esp_event.h>
#include <esp_sntp.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include <esp_hosted.h>
#endif
void NativeWifi::events(void *context,esp_event_base_t base,int32_t id,void *data){
 auto &self=*static_cast<NativeWifi*>(context);
 if(base==IP_EVENT&&id==IP_EVENT_STA_GOT_IP){
  self.link=WL_CONNECTED;
  portENTER_CRITICAL(&self.retryMutex);self.retry.connected();portEXIT_CRITICAL(&self.retryMutex);
 }
 else if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_CONNECTED&&data){auto *event=static_cast<wifi_event_sta_connected_t*>(data);self.rememberSsid(event->ssid,event->ssid_len);self.link=WL_IDLE_STATUS;portENTER_CRITICAL(&self.retryMutex);self.retry.next=0;portEXIT_CRITICAL(&self.retryMutex);}
 else if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_DISCONNECTED){
  unsigned reason=data?static_cast<wifi_event_sta_disconnected_t*>(data)->reason:0;
  self.disconnectReason=reason;self.link=WL_DISCONNECTED;
  printf("[WIFI] disconnected reason=%u\n",reason);
  if(self.wanted&&self.reconnect&&reason!=WIFI_REASON_ASSOC_LEAVE){
   portENTER_CRITICAL(&self.retryMutex);self.retry.disconnected(millis());portEXIT_CRITICAL(&self.retryMutex);
  }
 }
 else if(base==WIFI_EVENT&&id==WIFI_EVENT_SCAN_DONE){auto *event=static_cast<wifi_event_sta_scan_done_t*>(data);self.scan=event->status?WIFI_SCAN_FAILED:int(event->number);}
}
bool NativeWifi::init(){
 if(initialized)return true;
 auto error=esp_netif_init();if(error!=ESP_OK&&error!=ESP_ERR_INVALID_STATE)return false;
 error=esp_event_loop_create_default();if(error!=ESP_OK&&error!=ESP_ERR_INVALID_STATE)return false;
#if CONFIG_IDF_TARGET_ESP32P4
 // The board hook initializes Hosted; connect only after the default event loop.
 if(esp_hosted_connect_to_slave()!=ESP_OK)return false;
#endif
 if(!sta)sta=esp_netif_create_default_wifi_sta();
 if(!ap)ap=esp_netif_create_default_wifi_ap();
 if(!sta||!ap)return false;
 wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();if(esp_wifi_init(&cfg)!=ESP_OK)return false;
 esp_wifi_set_storage(WIFI_STORAGE_RAM);
 if(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,events,this)!=ESP_OK||esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,events,this)!=ESP_OK)return false;
 initialized=true;return true;
}
bool NativeWifi::mode(int m){if(!init())return false;if(!m){wanted=false;link=WL_DISCONNECTED;if(started)esp_wifi_stop();started=false;return true;}if(esp_wifi_set_mode(wifi_mode_t(m))!=ESP_OK)return false;if(!started){if(esp_wifi_start()!=ESP_OK)return false;started=true;}return true;}
bool NativeWifi::begin(const char *ssid,const char *password){
 wifi_config_t cfg{};size_t ssidBytes;
 if(!xtouch::network::fillWifiCredentials(cfg.sta.ssid,cfg.sta.password,ssid,password,ssidBytes))return false;
 if(!initialized&&!mode(WIFI_STA))return false;
 wanted=false;esp_wifi_scan_stop();scanDelete();esp_wifi_disconnect();cfg.sta.pmf_cfg.capable=true;rememberSsid(cfg.sta.ssid,ssidBytes);
 if(esp_wifi_set_config(WIFI_IF_STA,&cfg)!=ESP_OK)return false;
 portENTER_CRITICAL(&retryMutex);retry.begin(millis());portEXIT_CRITICAL(&retryMutex);
 wanted=true;link=WL_IDLE_STATUS;
 ++connectCalls;auto result=esp_wifi_connect();
 if(result!=ESP_OK){portENTER_CRITICAL(&retryMutex);retry.disconnected(millis());portEXIT_CRITICAL(&retryMutex);}
 return result==ESP_OK;
}
void NativeWifi::poll(){
 if(!wanted||!reconnect||link==WL_CONNECTED||scan==WIFI_SCAN_RUNNING)return;
 portENTER_CRITICAL(&retryMutex);bool due=retry.due(millis());portEXIT_CRITICAL(&retryMutex);
 if(due){
  link=WL_IDLE_STATUS;++connectCalls;auto result=esp_wifi_connect();
  if(result!=ESP_OK){portENTER_CRITICAL(&retryMutex);retry.disconnected(millis());portEXIT_CRITICAL(&retryMutex);}
 }
}
bool NativeWifi::disconnect(bool off,bool erase){wanted=false;link=WL_DISCONNECTED;portENTER_CRITICAL(&retryMutex);retry.stopped();portEXIT_CRITICAL(&retryMutex);auto rc=esp_wifi_disconnect();if(erase){wifi_config_t cfg{};esp_wifi_set_config(WIFI_IF_STA,&cfg);}if(off)mode(WIFI_OFF);return rc==ESP_OK;}
bool NativeWifi::softAP(const char *ssid,const char *password){wifi_config_t cfg{};size_t ssidBytes;if(!xtouch::network::fillWifiCredentials(cfg.ap.ssid,cfg.ap.password,ssid,password,ssidBytes)||(password&&*password&&strlen(password)<8))return false;if(!initialized&&!mode(WIFI_AP_STA))return false;cfg.ap.ssid_len=ssidBytes;cfg.ap.max_connection=4;cfg.ap.authmode=password&&*password?WIFI_AUTH_WPA2_PSK:WIFI_AUTH_OPEN;cfg.ap.channel=1;return esp_wifi_set_config(WIFI_IF_AP,&cfg)==ESP_OK;}
bool NativeWifi::softAPdisconnect(bool){return mode((getMode()&WIFI_STA)?WIFI_STA:WIFI_OFF);}
IPAddress NativeWifi::address(esp_netif_t *net)const{esp_netif_ip_info_t info{};if(net)esp_netif_get_ip_info(net,&info);return IPAddress(info.ip.addr);}
String NativeWifi::SSID(int index)const{if(index>=0)return unsigned(index)<networks.size()?String((char*)networks[index].ssid):String();char name[33];portENTER_CRITICAL(&ssidMutex);memcpy(name,stationSsid,sizeof(name));portEXIT_CRITICAL(&ssidMutex);return String(name);}
int32_t NativeWifi::RSSI(int index)const{if(index>=0)return unsigned(index)<networks.size()?networks[index].rssi:0;wifi_ap_record_t info{};return esp_wifi_sta_get_ap_info(&info)==ESP_OK?info.rssi:0;}
int NativeWifi::scanNetworks(bool async,bool hidden){if(!init())return WIFI_SCAN_FAILED;networks.clear();wifi_scan_config_t cfg{};cfg.show_hidden=hidden;scan=WIFI_SCAN_RUNNING;if(esp_wifi_scan_start(&cfg,!async)!=ESP_OK){scan=WIFI_SCAN_FAILED;return WIFI_SCAN_FAILED;}if(!async){uint16_t n=0;esp_wifi_scan_get_ap_num(&n);scan=n;}return async?WIFI_SCAN_RUNNING:scanComplete();}
int NativeWifi::scanComplete(){int n=scan;if(n<0)return n;if(networks.empty()&&n){uint16_t count=std::min(n,64);networks.resize(count);if(esp_wifi_scan_get_ap_records(&count,networks.data())!=ESP_OK)return WIFI_SCAN_FAILED;networks.resize(count);}return networks.size();}
void configTzTime(const char *zone,const char *server){setenv("TZ",zone,1);tzset();if(esp_sntp_enabled())esp_sntp_stop();esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);esp_sntp_setservername(0,const_cast<char*>(server));esp_sntp_init();}
