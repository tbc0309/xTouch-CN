#pragma once
#include <mutex>
#include <cstdint>
#include <cstring>
#include <cerrno>
namespace xtouch_idf {
// A retired card rejects new operations but remains registered until every
// existing file/directory has completed its close/flush path.
class CardIoGate {
 mutable std::mutex mutex;
 unsigned users=0;bool mounted=false,retired=false,probing=false;
 int type=0;uint64_t bytes=0;
public:
 void mount(int value,uint64_t size){std::lock_guard<std::mutex> lock(mutex);mounted=true;retired=false;probing=false;type=value;bytes=size;}
 bool acquire(){std::lock_guard<std::mutex> lock(mutex);if(!mounted||retired||probing)return false;++users;return true;}
 void release(){std::lock_guard<std::mutex> lock(mutex);if(users)--users;}
 void fault(){std::lock_guard<std::mutex> lock(mutex);if(mounted)retired=true;}
 bool retiring()const{std::lock_guard<std::mutex> lock(mutex);return mounted&&retired;}
 bool claimUnmount(){std::lock_guard<std::mutex> lock(mutex);if(!mounted)return false;retired=true;if(users||probing)return false;mounted=false;type=0;bytes=0;return true;}
 bool beginProbe(){std::lock_guard<std::mutex> lock(mutex);if(!mounted||retired||users||probing)return false;probing=true;return true;}
 void endProbe(bool success){std::lock_guard<std::mutex> lock(mutex);if(!success)retired=true;probing=false;}
 int mediaType()const{std::lock_guard<std::mutex> lock(mutex);return mounted&&!retired?type:0;}
 uint64_t mediaBytes()const{std::lock_guard<std::mutex> lock(mutex);return mounted&&!retired?bytes:0;}
};
inline CardIoGate &cardIoGate(){static CardIoGate gate;return gate;}
class CardLease {
 bool card=false,held=false;
public:
 explicit CardLease(const char *path):card(path&&!std::strncmp(path,"/sdcard",7)&&(path[7]=='/'||path[7]=='\0')){if(card)held=cardIoGate().acquire();}
 CardLease(CardLease&&other)noexcept:card(other.card),held(other.held){other.held=false;}
 CardLease(const CardLease&)=delete;CardLease&operator=(const CardLease&)=delete;
 ~CardLease(){if(held)cardIoGate().release();}
 explicit operator bool()const{return !card||held;}
 void noteError(int error=errno)const{if(held&&(error==EIO||error==ENODEV||error==ETIMEDOUT))cardIoGate().fault();}
};
}
