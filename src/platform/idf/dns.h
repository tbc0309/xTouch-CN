#ifndef XTOUCH_NATIVE_IDF_DNS_H
#define XTOUCH_NATIVE_IDF_DNS_H
#pragma once
#include "socket.h"
class DNSServer {
 int fd=-1;IPAddress answer;
public:
 ~DNSServer(){stop();}
 bool start(uint16_t port,const String &,IPAddress ip){stop();answer=ip;fd=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(fd<0)return false;sockaddr_in local{};local.sin_family=AF_INET;local.sin_port=htons(port);local.sin_addr.s_addr=uint32_t(ip);if(bind(fd,(sockaddr*)&local,sizeof(local))){stop();return false;}fcntl(fd,F_SETFL,O_NONBLOCK);return true;}
 void stop(){if(fd>=0){::close(fd);fd=-1;}}
 void processNextRequest(){if(fd<0)return;uint8_t b[512];sockaddr_in peer{};socklen_t size=sizeof(peer);int n=recvfrom(fd,b,sizeof(b)-16,0,(sockaddr*)&peer,&size);if(n<17||(b[2]&0x80)||b[4]!=0||b[5]!=1)return;int end=12;while(end<n&&b[end]){unsigned len=b[end];if(len>63||end+len+1>=n)return;end+=len+1;}if(end+5>n)return;end+=5;bool ipv4=b[end-4]==0&&b[end-3]==1&&b[end-2]==0&&b[end-1]==1;b[2]=0x81;b[3]=0x80;b[6]=0;b[7]=ipv4?1:0;b[8]=b[9]=b[10]=b[11]=0;int used=end;if(ipv4){uint8_t rr[]={0xc0,0x0c,0,1,0,1,0,0,0,10,0,4,answer[0],answer[1],answer[2],answer[3]};memcpy(b+used,rr,sizeof(rr));used+=sizeof(rr);}sendto(fd,b,used,0,(sockaddr*)&peer,size);}
};

#endif
