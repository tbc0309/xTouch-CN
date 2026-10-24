#pragma once
#include <cstring>
#include <cstdio>

namespace XTouchProfileRoute {
inline bool local(bool configured,bool cloud) {return configured&&!cloud;}
inline bool cameraLink(const char *serial,const char *linkedHost,bool candidateConfigured,
                       bool candidateCloud,const char *candidateSerial,const char *candidateHost) {
 return serial&&*serial&&linkedHost&&local(candidateConfigured,candidateCloud)&&
  candidateSerial&&candidateHost&&!std::strcmp(serial,candidateSerial)&&(!*linkedHost||!std::strcmp(linkedHost,candidateHost));
}
inline bool sameCamera(const char *host,const char *code,const char *otherHost,const char *otherCode) {
 return !std::strcmp(host,otherHost)&&!std::strcmp(code,otherCode);
}
inline bool lanHost(const char *host) {
 unsigned a,b,c,d;char extra;
 return host&&std::sscanf(host,"%u.%u.%u.%u%c",&a,&b,&c,&d,&extra)==4&&a<=255&&b<=255&&c<=255&&d<=255&&
  (a==10||(a==172&&b>=16&&b<=31)||(a==192&&b==168));
}
template<class Profile>const Profile *cameraProfile(const Profile *profiles,size_t count,size_t selected) {
 if(!profiles||selected>=count||!profiles[selected].configured)return nullptr;
 const auto &active=profiles[selected];
 if(!active.cloud)return lanHost(active.host)&&std::strlen(active.accessCode)==8?&active:nullptr;
 const Profile *match=nullptr;
 for(size_t slot=0;slot<count;++slot){const auto &candidate=profiles[slot];
  if(!cameraLink(active.serialNumber,active.cameraLanHost,candidate.configured,candidate.cloud,candidate.serialNumber,candidate.host)||
     !lanHost(candidate.host)||std::strlen(candidate.accessCode)!=8)continue;
  if(match&&!sameCamera(match->host,match->accessCode,candidate.host,candidate.accessCode))return nullptr;
  match=&candidate;
 }
 return match;
}
}
