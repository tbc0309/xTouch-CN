#pragma once
#include <cstring>
#include "xtouch/cloud_endpoints.h"

namespace XTouchPreview {
enum class Source {Local,Cloud,Simulator};
inline bool validTaskId(const char *id) {
 if(!id||!*id||!std::strcmp(id,"0"))return false;
 unsigned n=0;for(;id[n];++n)if(n>=32||id[n]<'0'||id[n]>'9')return false;
 return true;
}
inline const char *cloudTaskId(const char *subtask,const char *task) {
 // An explicit zero means this print has no cloud subtask. Only legacy
 // reports which omit the field may fall back to their task ID.
 if(subtask&&*subtask)return validTaskId(subtask)?subtask:"";
 return validTaskId(task)?task:"";
}
inline Source source(bool cloud,bool simulator) {
 return cloud?(simulator?Source::Simulator:Source::Cloud):Source::Local;
}
inline bool matchesTask(const char *id,const char *serial,unsigned plate,
                        const char *expectedId,const char *expectedSerial,unsigned expectedPlate) {
 return id&&*id&&std::strcmp(id,"0")&&serial&&*serial&&expectedId&&expectedSerial&&
  plate&&plate==expectedPlate&&!std::strcmp(id,expectedId)&&!std::strcmp(serial,expectedSerial);
}
inline const char *taskApi(bool china) {
 return XTouchCloudEndpoints::get(china?"cn":"global")->taskHistory;
}
// A missing cloud task may appear late. Back off after the initial attempts,
// without repeatedly retrying invalid images, untrusted URLs or credentials.
inline unsigned retryDelay(int result,int httpStatus,unsigned attempts) {
 if(!attempts||attempts>=6)return 0;
 const bool transient=result==3||result==5||result==6||result==13||result==25||
  (result==26&&(httpStatus<0||httpStatus==408||httpStatus==429||httpStatus>=500))||
  (result==27&&httpStatus==-2);
 return transient?(attempts<3?30000:120000):0;
}
inline bool trustedCoverHost(const char *host) {
 if(!host||!*host)return false;
 const size_t length=std::strlen(host);
 const auto ends=[&](const char *suffix){const size_t n=std::strlen(suffix);return length>n&&!std::strcmp(host+length-n,suffix);};
 return !std::strcmp(host,"public-cdn.bblmw.cn")||!std::strcmp(host,"public-cdn.bblmw.com")||!std::strcmp(host,"makerworld.bblmw.com")||
  ends(".bambulab.com")||ends(".bambulab.cn")||!std::strcmp(host,"bbl-prod-model.oss-cn-shanghai.aliyuncs.com")||
  !std::strcmp(host,"bbl-us-public.oss-us-west-1.aliyuncs.com")||
  (!std::strncmp(host,"bbl-prod-model.s3.",18)&&ends(".amazonaws.com"));
}
}
