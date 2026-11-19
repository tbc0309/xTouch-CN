#pragma once
#include "policy.h"
namespace xtouch_files {
inline bool transientCoverError(int error){return error==3||error==5||error==6||error==13;}
inline uint32_t coverKey(const std::string &path){uint32_t value=2166136261u;for(unsigned char c:std::string("/sdcard")+path)value=(value^c)*16777619u;return value;}
inline std::string printerCover(const std::string &path,unsigned plate,bool digest=false){
 if(!pathValid(path)||!plate||plate>999)return {};
 return std::string(digest?"/image/md5/":"/image/")+std::to_string(coverKey(path))+std::to_string(plate)+(digest?".md5":".png");
}
inline bool extractedPlate(const std::string &path,std::string &title,unsigned &plate){
 if(!suffix(path,".gcode"))return false;auto slash=path.find_last_of('/'),at=path.rfind("_plate_");
 if(at==std::string::npos||at<=slash+1)return false;
 std::string digits=path.substr(at+7,path.size()-at-7-6);if(digits.empty()||digits.size()>3)return false;
 plate=0;for(char c:digits){if(c<'0'||c>'9')return false;plate=plate*10+c-'0';}
 if(!plate||plate>999)return false;title=path.substr(slash+1,at-slash-1);return true;
}
inline bool sourceCandidate(const std::string &raw,const std::string &package,unsigned &plate){
 if(raw.rfind("/cache/",0)!=0||!suffix(package,".3mf"))return false;
 std::string title;if(!extractedPlate(raw,title,plate))return false;
 std::string name=package.substr(package.find_last_of('/')+1),base=name.substr(0,name.size()-4);
 if(suffix(base,".gcode"))base.resize(base.size()-6);
 return title==base || title==name.substr(0,name.find('.'));
}
}
