#ifndef XTOUCH_NATIVE_IDF_FILESYSTEM_H
#define XTOUCH_NATIVE_IDF_FILESYSTEM_H
#pragma once
#include "runtime.h"
#include "card_io_guard.h"
#include <climits>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <esp_spiffs.h>
#include <sdmmc_cmd.h>
#include <sd_protocol_defs.h>
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"
constexpr int CARD_NONE=0,CARD_MMC=1,CARD_SD=2,CARD_SDHC=3;
enum SeekMode {SeekSet=SEEK_SET,SeekCur=SEEK_CUR,SeekEnd=SEEK_END};
namespace fs {
class File:public Stream {
 struct State {
  xtouch_idf::CardLease access;FILE *file=nullptr;DIR *dir=nullptr;String path;
  explicit State(xtouch_idf::CardLease&&lease):access(std::move(lease)){}
  ~State(){if(file&&fclose(file))access.noteError();if(dir&&closedir(dir))access.noteError();}
 };
 std::shared_ptr<State> state;
public:
 File()=default;
 File(const String &path,const char *mode){
  xtouch_idf::CardLease lease(path.c_str());if(!lease)return;
  auto value=std::make_shared<State>(std::move(lease));value->path=path;struct stat info{};
  if(!stat(path.c_str(),&info)&&S_ISDIR(info.st_mode))value->dir=opendir(path.c_str());else value->file=fopen(path.c_str(),mode);
  if(value->file||value->dir)state=std::move(value);else value->access.noteError();
 }
 explicit operator bool()const{return state&&(state->file||state->dir);}
 bool isDirectory()const{return state&&state->dir;}
 size_t size()const{if(!state||!state->file)return 0;struct stat info{};if(fstat(fileno(state->file),&info)){state->access.noteError();return 0;}return info.st_size;}
 time_t getLastWrite()const{if(!state)return 0;struct stat info{};if(stat(state->path.c_str(),&info)){state->access.noteError();return 0;}return info.st_mtime;}
 size_t position()const{return state&&state->file?ftell(state->file):0;}
 bool seek(uint32_t at,SeekMode mode=SeekSet){if(!state||!state->file)return false;if(fseek(state->file,at,int(mode))){state->access.noteError();return false;}return true;}
 const char *path()const{return state?state->path.c_str():"";}
 const char *name()const{auto p=strrchr(path(),'/');return p?p+1:path();}
 int available()override{if(!state||!state->file)return 0;auto bytes=size(),at=position();return bytes>at?int(std::min<size_t>(bytes-at,INT_MAX)):0;}
 int read()override{if(!state||!state->file)return -1;int c=fgetc(state->file);if(c<0&&ferror(state->file))state->access.noteError();return c;}
 size_t read(uint8_t *out,size_t n){if(!state||!state->file)return 0;auto count=fread(out,1,n,state->file);if(count<n&&ferror(state->file))state->access.noteError();return count;}
 size_t readBytes(uint8_t *out,size_t n){return read(out,n);}
 size_t readBytes(char *out,size_t n){return read((uint8_t*)out,n);}
 int peek()override{int c=read();if(c>=0)ungetc(c,state->file);return c;}
 using Print::write;
 size_t write(uint8_t c)override{return write(&c,1);}
 size_t write(const uint8_t *p,size_t n)override{if(!state||!state->file)return 0;auto count=fwrite(p,1,n,state->file);if(count<n)state->access.noteError();return count;}
 void flush()override{if(state&&state->file){if(fflush(state->file)){state->access.noteError();return;}if(fsync(fileno(state->file)))state->access.noteError();}}
 void close(){state.reset();}
 File openNextFile(const char *mode="r"){if(!isDirectory())return {};while(auto e=readdir(state->dir)){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;return File(state->path+"/"+e->d_name,mode);}return {};}
};
class FS {
protected:String root;
 String full(const String &p)const{return root+p;}
public:
 explicit FS(const char *path):root(path){}
 File open(const String &path,const char *mode="r"){return File(full(path),mode);}
 bool exists(const String &path){auto p=full(path);xtouch_idf::CardLease lease(p.c_str());if(!lease)return false;struct stat info{};bool ok=!stat(p.c_str(),&info);if(!ok)lease.noteError();return ok;}
 bool remove(const String &path){auto p=full(path);xtouch_idf::CardLease lease(p.c_str());if(!lease)return false;bool ok=!unlink(p.c_str());if(!ok)lease.noteError();return ok;}
 bool rename(const String &from,const String &to){auto p=full(from);xtouch_idf::CardLease lease(p.c_str());if(!lease)return false;bool ok=!::rename(p.c_str(),full(to).c_str());if(!ok)lease.noteError();return ok;}
 bool mkdir(const String &path){auto p=full(path);xtouch_idf::CardLease lease(p.c_str());if(!lease)return false;bool ok=!::mkdir(p.c_str(),0770)||errno==EEXIST;if(!ok)lease.noteError();return ok;}
 bool rmdir(const String &path){auto p=full(path);xtouch_idf::CardLease lease(p.c_str());if(!lease)return false;bool ok=!::rmdir(p.c_str());if(!ok)lease.noteError();return ok;}

};
}
using fs::File;
using fs::FS;
class NativeSpiffs:public fs::FS {
public:
 NativeSpiffs():FS("/spiffs"){}
 bool begin(bool formatOnFailure=false){esp_vfs_spiffs_conf_t c{};c.base_path="/spiffs";c.partition_label="spiffs";c.max_files=10;c.format_if_mount_failed=formatOnFailure;auto rc=esp_vfs_spiffs_register(&c);return rc==ESP_OK||rc==ESP_ERR_INVALID_STATE;}
 bool format(){return esp_spiffs_format("spiffs")==ESP_OK;}
 size_t totalBytes()const{size_t total=0,used=0;esp_spiffs_info("spiffs",&total,&used);return total;}
 size_t usedBytes()const{size_t total=0,used=0;esp_spiffs_info("spiffs",&total,&used);return used;}
};
enum class CardProbeResult {Busy,Ready,Failed};
class NativeCard:public fs::FS {
 sdmmc_card_t *card=nullptr;
public:
 NativeCard():FS("/sdcard"){}
 bool begin();
 void end();
 void poll(){if(xtouch_idf::cardIoGate().retiring())end();}
 int cardType()const{return xtouch_idf::cardIoGate().mediaType();}
 uint64_t cardSize()const{return xtouch_idf::cardIoGate().mediaBytes();}
 CardProbeResult probeRaw(uint8_t *p,uint32_t sector);
 bool readRAW(uint8_t *p,uint32_t sector){return probeRaw(p,sector)==CardProbeResult::Ready;}
};
inline NativeSpiffs SPIFFS;
inline NativeCard SD_MMC;

#endif
