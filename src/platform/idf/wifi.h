#ifndef XTOUCH_NATIVE_IDF_WIFI_H
#define XTOUCH_NATIVE_IDF_WIFI_H
#pragma once
#include "runtime.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <atomic>
#include "features/network/wifi_retry_policy.h"
constexpr int WIFI_OFF=0,WIFI_STA=1,WIFI_AP=2,WIFI_AP_STA=3;
constexpr int WIFI_SCAN_RUNNING=-1,WIFI_SCAN_FAILED=-2,WIFI_POWER_19_5dBm=78;
enum wl_status_t {WL_IDLE_STATUS=0,WL_NO_SSID_AVAIL=1,WL_SCAN_COMPLETED=2,WL_CONNECTED=3,WL_CONNECT_FAILED=4,WL_CONNECTION_LOST=5,WL_DISCONNECTED=6};
class IPAddress {
 uint32_t address=0;
public:
 IPAddress()=default;IPAddress(uint32_t value):address(value){}
 IPAddress(uint8_t a,uint8_t b,uint8_t c,uint8_t d){uint8_t p[4]={a,b,c,d};memcpy(&address,p,4);}
 operator uint32_t()const{return address;}
 bool operator==(const IPAddress &other)const{return address==other.address;}
 bool operator!=(const IPAddress &other)const{return address!=other.address;}
 uint8_t operator[](unsigned n)const{return reinterpret_cast<const uint8_t*>(&address)[n];}
 bool fromString(const char *s){return s&&inet_pton(AF_INET,s,&address)==1;}
 operator String()const{return toString();}
 String toString()const{char text[16];inet_ntop(AF_INET,&address,text,sizeof(text));return text;}
};
class NativeWifi {
 bool initialized=false,started=false;
 mutable portMUX_TYPE ssidMutex=portMUX_INITIALIZER_UNLOCKED;
 char stationSsid[33]{};
 void rememberSsid(const uint8_t *value,size_t n){portENTER_CRITICAL(&ssidMutex);n=std::min(n,size_t(32));memcpy(stationSsid,value,n);stationSsid[n]=0;portEXIT_CRITICAL(&ssidMutex);}
 std::atomic<bool> reconnect{true},wanted{false};
 std::atomic<wl_status_t> link{WL_DISCONNECTED};
 std::atomic<int> scan{WIFI_SCAN_FAILED};
 mutable portMUX_TYPE retryMutex=portMUX_INITIALIZER_UNLOCKED;
 XTouchWifiRetry retry;
 std::atomic<unsigned> disconnectReason{0};
 std::atomic<unsigned> connectCalls{0};
 esp_netif_t *sta=nullptr,*ap=nullptr;
 std::vector<wifi_ap_record_t> networks;
 static void events(void *,esp_event_base_t,int32_t,void *);
 bool init();
 IPAddress address(esp_netif_t *net)const;
public:
 bool mode(int mode);
 int getMode()const{wifi_mode_t m=WIFI_MODE_NULL;esp_wifi_get_mode(&m);return m;}
 void persistent(bool){}
 void setAutoReconnect(bool enabled){reconnect=enabled;}
 void resumeAutoReconnect(){reconnect=true;wanted=true;portENTER_CRITICAL(&retryMutex);retry.connected();portEXIT_CRITICAL(&retryMutex);}
 void poll();
 bool retryExhausted()const{portENTER_CRITICAL(&retryMutex);bool value=retry.exhausted;portEXIT_CRITICAL(&retryMutex);return value;}
 uint32_t connectionStartedAt()const{portENTER_CRITICAL(&retryMutex);uint32_t value=retry.started;portEXIT_CRITICAL(&retryMutex);return value;}
 unsigned connectionAttempts()const{portENTER_CRITICAL(&retryMutex);unsigned value=retry.attempts;portEXIT_CRITICAL(&retryMutex);return value;}
 unsigned lastDisconnectReason()const{return disconnectReason;}
 unsigned totalConnectionAttempts()const{return connectCalls;}
 wl_status_t status()const{return link.load();}
 bool begin(const char *ssid,const char *password);
 bool disconnect(bool off=false,bool erase=false);
 bool softAP(const char *ssid,const char *password);
 bool softAPdisconnect(bool off=false);
 IPAddress localIP()const{return address(sta);}
 IPAddress softAPIP()const{return address(ap);}
 String SSID(int index=-1)const;
 int32_t RSSI(int index=-1)const;
 wifi_auth_mode_t encryptionType(int index)const{return index>=0&&unsigned(index)<networks.size()?networks[index].authmode:WIFI_AUTH_OPEN;}
 int scanNetworks(bool async=true,bool hidden=true);
 int scanComplete();
 void scanDelete(){networks.clear();scan=WIFI_SCAN_FAILED;}
 bool setSleep(bool value){return esp_wifi_set_ps(value?WIFI_PS_MIN_MODEM:WIFI_PS_NONE)==ESP_OK;}
 bool setSleep(wifi_ps_type_t value){return esp_wifi_set_ps(value)==ESP_OK;}
 wifi_ps_type_t getSleep()const{wifi_ps_type_t mode=WIFI_PS_NONE;esp_wifi_get_ps(&mode);return mode;}
 bool setTxPower(int value){return esp_wifi_set_max_tx_power(value)==ESP_OK;}
};
inline NativeWifi WiFi;
void configTzTime(const char *zone,const char *server);

#endif
