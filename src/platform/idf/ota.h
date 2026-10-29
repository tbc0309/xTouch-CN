#ifndef XTOUCH_NATIVE_IDF_OTA_H
#define XTOUCH_NATIVE_IDF_OTA_H
#pragma once
#include "filesystem.h"
#include "digest.h"
#include "heap_buffer.h"
#include <esp_ota_ops.h>
#include <esp_app_desc.h>
#include "xtouch/firmware_image.h"
constexpr size_t UPDATE_SIZE_UNKNOWN=SIZE_MAX;
constexpr int U_FLASH=0,U_SPIFFS=100;
class NativeUpdater {
 esp_ota_handle_t handle=0;
 const esp_partition_t *partition=nullptr;
 size_t expected=0,written=0;
 esp_err_t error=ESP_OK;
 MD5Builder hash;String md5;
 std::function<void(size_t,size_t)> progress;
public:
 void onProgress(std::function<void(size_t,size_t)> callback){progress=callback;}
 bool begin(size_t size=UPDATE_SIZE_UNKNOWN,int command=U_FLASH){abort();error=ESP_OK;written=0;expected=size;md5="";partition=esp_ota_get_next_update_partition(nullptr);if(command!=U_FLASH||!partition||(size!=UPDATE_SIZE_UNKNOWN&&size>partition->size)){error=ESP_ERR_INVALID_SIZE;return false;}error=esp_ota_begin(partition,size==UPDATE_SIZE_UNKNOWN?OTA_SIZE_UNKNOWN:size,&handle);hash.begin();return error==ESP_OK;}
 bool setMD5(const char *value){if(!value||strlen(value)!=32)return false;md5=value;md5.toLowerCase();return true;}
 size_t write(const uint8_t *p,size_t n){if(!handle||error!=ESP_OK)return 0;if(expected!=UPDATE_SIZE_UNKNOWN&&n>expected-written){error=ESP_ERR_INVALID_SIZE;return 0;}error=esp_ota_write(handle,p,n);if(error!=ESP_OK)return 0;hash.add(p,n);written+=n;if(progress)progress(written,expected);return n;}
 template<class Source>size_t writeStream(Source &source){size_t total=0;NativeHeapBuffer b(2048, NativeBufferMemory::PreferInternal);if(!b){error=ESP_ERR_NO_MEM;return 0;}while(source.available()){size_t n=source.readBytes(b.data(),std::min(b.size(),size_t(source.available())));if(!n)break;size_t done=write(b.data(),n);total+=done;if(done!=n)break;}return total;}
 bool end(bool acceptPartial=false){
  if(!handle||error!=ESP_OK||(!acceptPartial&&expected!=UPDATE_SIZE_UNKNOWN&&written!=expected)){abort();return false;}
  hash.calculate();if(!md5.empty()&&hash.toString()!=md5){error=ESP_ERR_INVALID_CRC;abort();return false;}
  auto h=handle;handle=0;error=esp_ota_end(h);
  if(error==ESP_OK){
   esp_app_desc_t candidate{};error=esp_ota_get_partition_description(partition,&candidate);
   if(error==ESP_OK&&!xtouch_application_project(candidate.project_name,esp_app_get_description()->project_name))error=ESP_ERR_OTA_VALIDATE_FAILED;
  }
  if(error==ESP_OK)error=esp_ota_set_boot_partition(partition);
  return error==ESP_OK;
 }
 void abort(){if(handle){esp_ota_abort(handle);handle=0;}}
 bool isRunning()const{return handle!=0;}
 bool hasError()const{return error!=ESP_OK;}
 unsigned getError()const{return error;}
 void printError(Print &out)const{out.println(esp_err_to_name(error));}
};
inline NativeUpdater Update;

#endif
