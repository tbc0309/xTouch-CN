#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <algorithm>
namespace xtouch_files {
struct PlateStats {
 unsigned plate=0;uint32_t seconds=0;double grams=0;
 bool hasTime=false,hasWeight=false;
};
// Read the numeric subset of slice_info XML. No entities, DTDs or external I/O.
// Parse the entire bounded member before accepting any per-plate values.
inline bool sliceStats(const char *data,size_t size,std::vector<PlateStats> &out){
 out.clear();if(!data||!size||size>65536)return false;
 std::string xml(data,size);std::vector<std::string> stack;std::vector<PlateStats> parsed;
 PlateStats current;bool inPlate=false,rootSeen=false;size_t at=0;
 auto space=[](char c){return c==' '||c=='\t'||c=='\r'||c=='\n';};
 auto nameChar=[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c==':'||c=='-'||c=='.';};
 while(at<size){
  if(xml[at]!='<'){if(!space(xml[at]))return false;++at;continue;}
  if(xml.compare(at,4,"<!--")==0){auto end=xml.find("-->",at+4);if(end==std::string::npos)return false;at=end+3;continue;}
  if(xml.compare(at,2,"<?")==0){auto end=xml.find("?>",at+2);if(end==std::string::npos)return false;at=end+2;continue;}
  ++at;bool closing=at<size&&xml[at]=='/';if(closing)++at;
  size_t begin=at;while(at<size&&nameChar(xml[at]))++at;if(at==begin)return false;
  std::string tag=xml.substr(begin,at-begin),key,value;bool sawKey=false,sawValue=false,self=false;
  while(at<size){
   while(at<size&&space(xml[at]))++at;if(at==size)return false;
   if(xml[at]=='>'){++at;break;}
   if(xml[at]=='/'&&at+1<size&&xml[at+1]=='>'){self=true;at+=2;break;}
   if(closing)return false;
   begin=at;while(at<size&&nameChar(xml[at]))++at;if(at==begin)return false;
   std::string attr=xml.substr(begin,at-begin);while(at<size&&space(xml[at]))++at;
   if(at==size||xml[at++]!='=')return false;while(at<size&&space(xml[at]))++at;
   if(at==size||(xml[at]!='\''&&xml[at]!='"'))return false;char quote=xml[at++];begin=at;
   while(at<size&&xml[at]!=quote){if(xml[at]=='<')return false;++at;}if(at==size)return false;
   std::string v=xml.substr(begin,at++-begin);
   if(attr=="key"){if(sawKey)return false;sawKey=true;key=v;}
   if(attr=="value"){if(sawValue)return false;sawValue=true;value=v;}
  }
  if(at>size||xml[at-1]!='>')return false;
  if(closing){
   if(self||stack.empty()||stack.back()!=tag)return false;
   if(tag=="plate"){
    if(!inPlate||!current.plate||current.plate>999)return false;
    for(const auto &s:parsed)if(s.plate==current.plate)return false;
    parsed.push_back(current);inPlate=false;
   }
   stack.pop_back();continue;
  }
  if(stack.empty()){if(tag!="config"||rootSeen)return false;rootSeen=true;}
  if(tag=="plate"){
   if(inPlate||self||stack.size()!=1||stack.back()!="config")return false;
   current={};inPlate=true;
  }
  if(tag=="metadata"&&inPlate&&stack.back()=="plate"&&sawKey&&sawValue){
   if(key=="index"||key=="prediction"||key=="weight"){
    if(value.empty()||value.find_first_not_of("0123456789.")!=std::string::npos)return false;
    char *end=nullptr;double n=std::strtod(value.c_str(),&end);
    if(!end||*end||!std::isfinite(n)||n<0)return false;
    if(key=="index"){if(current.plate||n<1||n>999||n!=std::floor(n))return false;current.plate=unsigned(n);}
    if(key=="prediction"){if(current.hasTime||n>4294967295.0)return false;current.seconds=uint32_t(n);current.hasTime=true;}
    if(key=="weight"){if(current.hasWeight)return false;current.grams=n;current.hasWeight=true;}
   }
  }
  if(!self){if(stack.size()>=32)return false;stack.push_back(tag);}
 }
 if(!rootSeen||!stack.empty()||inPlate)return false;out.swap(parsed);return true;
}
inline std::string statsCaption(uint32_t seconds,bool hasTime,double grams,bool hasWeight){
 char b[80];std::string out;
 if(hasTime){unsigned m=seconds/60,h=m/60,d=h/24;h%=24;m%=60;
  if(d)std::snprintf(b,sizeof(b),"%ud %uh %um",d,h,m);
  else if(h)std::snprintf(b,sizeof(b),"%uh %um",h,m);
  else std::snprintf(b,sizeof(b),"%um",m);out=b;}
 if(hasWeight){std::snprintf(b,sizeof(b),"%.1f g",grams);if(!out.empty())out+="  ";out+=b;}
 return out;
}
}
