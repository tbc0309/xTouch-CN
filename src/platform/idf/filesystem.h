#ifndef XTOUCH_NATIVE_IDF_FILESYSTEM_H
#define XTOUCH_NATIVE_IDF_FILESYSTEM_H
#pragma once
#include "runtime.h"
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
 struct State {FILE *file=nullptr;DIR *dir=nullptr;String path;~State(){if(file)fclose(file);if(dir)closedir(dir);}};
 std::shared_ptr<State> state;
public:
 File()=default;
 File(const String &path,const char *mode){struct stat info{};if(!stat(path.c_str(),&info)&&S_ISDIR(info.st_mode)){auto dir=opendir(path.c_str());if(dir){state=std::make_shared<State>();state->dir=dir;state->path=path;}}else if(auto file=fopen(path.c_str(),mode)){state=std::make_shared<State>();state->file=file;state->path=path;}}
 explicit operator bool()const{return state&&(state->file||state->dir);}
 bool isDirectory()const{return state&&state->dir;}
 size_t size()const{if(!state||!state->file)return 0;struct stat info{};return fstat(fileno(state->file),&info)?0:info.st_size;}
 time_t getLastWrite()const{struct stat info{};return state&&!stat(state->path.c_str(),&info)?info.st_mtime:0;}
 size_t position()const{return state&&state->file?ftell(state->file):0;}
 bool seek(uint32_t at,SeekMode mode=SeekSet){return state&&state->file&&!fseek(state->file,at,int(mode));}
 const char *path()const{return state?state->path.c_str():"";}
 const char *name()const{auto p=strrchr(path(),'/');return p?p+1:path();}
 int available()override{return state&&state->file?std::max(0,int(size()-position())):0;}
 int read()override{return state&&state->file?fgetc(state->file):-1;}
 size_t read(uint8_t *out,size_t n){return state&&state->file?fread(out,1,n,state->file):0;}
 size_t readBytes(uint8_t *out,size_t n){return read(out,n);}
 size_t readBytes(char *out,size_t n){return read((uint8_t*)out,n);}
 int peek()override{int c=read();if(c>=0)ungetc(c,state->file);return c;}
 using Print::write;
 size_t write(uint8_t c)override{return write(&c,1);}
 size_t write(const uint8_t *p,size_t n)override{return state&&state->file?fwrite(p,1,n,state->file):0;}
 void flush()override{if(state&&state->file){fflush(state->file);fsync(fileno(state->file));}}
 void close(){state.reset();}
 File openNextFile(const char *mode="r"){if(!isDirectory())return {};while(auto e=readdir(state->dir)){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;return File(state->path+"/"+e->d_name,mode);}return {};}
};
class FS {
protected:String root;
 String full(const String &p)const{return root+p;}
public:
 explicit FS(const char *path):root(path){}
 File open(const String &path,const char *mode="r"){return File(full(path),mode);}
 bool exists(const String &path){struct stat info{};return !stat(full(path).c_str(),&info);}
 bool remove(const String &path){return !unlink(full(path).c_str());}
 bool rename(const String &from,const String &to){return !::rename(full(from).c_str(),full(to).c_str());}
 bool mkdir(const String &path){return !::mkdir(full(path).c_str(),0770)||errno==EEXIST;}
 bool rmdir(const String &path){return !::rmdir(full(path).c_str());}
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
class NativeCard:public fs::FS {
 sdmmc_card_t *card=nullptr;
public:
 NativeCard():FS("/sdcard"){}
 bool begin();
 void end();
 int cardType()const{return card?(card->ocr&SD_OCR_SDHC_CAP?CARD_SDHC:CARD_SD):CARD_NONE;}
 uint64_t cardSize()const{return card?uint64_t(card->csd.capacity)*card->csd.sector_size:0;}
 bool readRAW(uint8_t *p,uint32_t sector){return card&&sdmmc_read_sectors(card,p,sector,1)==ESP_OK;}
};
inline NativeSpiffs SPIFFS;
inline NativeCard SD_MMC;

#endif
