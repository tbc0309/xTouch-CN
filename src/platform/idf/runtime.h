#ifndef XTOUCH_NATIVE_IDF_RUNTIME_H
#define XTOUCH_NATIVE_IDF_RUNTIME_H
#pragma once
#include "text.h"
#include <esp_timer.h>
#include <esp_random.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_mac.h>
#include <esp_clk_tree.h>
#include <soc/clk_tree_defs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_rom_sys.h>
#ifdef __cplusplus
constexpr int LOW=0,HIGH=1,INPUT=0,OUTPUT=1,INPUT_PULLUP=2,INPUT_PULLDOWN=3;
inline uint32_t millis(){return uint32_t(esp_timer_get_time()/1000);}
inline uint32_t micros(){return uint32_t(esp_timer_get_time());}
inline void delay(unsigned ms){vTaskDelay(std::max(TickType_t(1),pdMS_TO_TICKS(ms)));}
inline void delayMicroseconds(unsigned us){esp_rom_delay_us(us);}
inline void yield(){taskYIELD();}
inline long random(long upper){return upper>0?esp_random()%upper:0;}
inline long random(long lower,long upper){return lower+random(upper-lower);}
inline void randomSeed(unsigned long){}
inline void pinMode(int pin,int mode){gpio_config_t c{};c.pin_bit_mask=1ULL<<pin;c.mode=mode==OUTPUT?GPIO_MODE_OUTPUT:GPIO_MODE_INPUT;c.pull_up_en=mode==INPUT_PULLUP?GPIO_PULLUP_ENABLE:GPIO_PULLUP_DISABLE;c.pull_down_en=mode==INPUT_PULLDOWN?GPIO_PULLDOWN_ENABLE:GPIO_PULLDOWN_DISABLE;gpio_config(&c);}
inline void digitalWrite(int pin,int value){gpio_set_level(gpio_num_t(pin),value);}
inline int digitalRead(int pin){return gpio_get_level(gpio_num_t(pin));}
inline uint32_t getCpuFrequencyMhz(){uint32_t hz=0;esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU,ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,&hz);return hz/1000000;}
inline bool psramFound(){return heap_caps_get_total_size(MALLOC_CAP_SPIRAM)>0;}
inline void *ps_malloc(size_t size){return heap_caps_malloc(size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
inline void *ps_calloc(size_t n,size_t size){return heap_caps_calloc(n,size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
class NativeConsole:public Print {
public:
    void begin(unsigned=115200){}
    void setDebugOutput(bool){}
    operator bool()const{return true;}
    int available(){return 0;}
    int read(){return -1;}
    using Print::write;
    size_t write(uint8_t c)override{return fwrite(&c,1,1,stdout);}
    size_t write(const uint8_t *p,size_t n)override{return fwrite(p,1,n,stdout);}
    void flush(){fflush(stdout);}
};
inline NativeConsole Serial;
struct NativeSystem {
    uint64_t getEfuseMac()const{uint8_t mac[6]{};esp_efuse_mac_get_default(mac);uint64_t value=0;memcpy(&value,mac,6);return value;}
    uint32_t getFreeHeap()const{return esp_get_free_heap_size();}
    uint32_t getMinFreeHeap()const{return esp_get_minimum_free_heap_size();}
    uint32_t getCpuFreqMHz()const{return getCpuFrequencyMhz();}
    [[noreturn]] void restart()const{esp_restart();}
};
inline NativeSystem ESP;
inline long map(long value,long fromMin,long fromMax,long toMin,long toMax){return (value-fromMin)*(toMax-toMin)/(fromMax-fromMin)+toMin;}
template<class T>inline T constrain(T value,T lo,T hi){return std::min(hi,std::max(lo,value));}
using std::min;using std::max;
bool ledcAttach(uint8_t pin,uint32_t frequency,uint8_t resolution);
bool ledcWrite(uint8_t pin,uint32_t duty);
inline size_t Stream::readBytes(uint8_t *out,size_t n){size_t used=0;uint32_t at=millis();while(used<n&&millis()-at<timeoutMs){int c=read();if(c>=0){out[used++]=c;at=millis();}else delay(1);}return used;}
inline String Stream::readStringUntil(char end){String out;uint32_t at=millis();while(millis()-at<timeoutMs){int c=read();if(c<0){delay(1);continue;}if(c==end)break;out+=char(c);at=millis();}return out;}
inline String Stream::readString(){String out;while(available()){int c=read();if(c<0)break;out+=char(c);}return out;}
inline bool Stream::find(const char *needle){size_t matched=0,n=strlen(needle);uint32_t start=millis();while(n&&millis()-start<timeoutMs){int c=read();if(c<0){delay(1);continue;}matched=c==needle[matched]?matched+1:0;if(matched==n)return true;}return !n;}
#endif

#endif
