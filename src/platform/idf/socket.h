#ifndef XTOUCH_NATIVE_IDF_SOCKET_H
#define XTOUCH_NATIVE_IDF_SOCKET_H
#pragma once
#include "wifi.h"
#include "tls_receive.h"
#include "socket_timeout.h"
#include <esp_tls.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <fcntl.h>
#include <memory>
class WiFiClient:public Stream {
protected:
 struct Socket {int fd=-1;esp_tls_t *tls=nullptr;bool live=false;uint8_t received[2048]{};size_t cursor=0,count=0;~Socket(){if(tls)esp_tls_conn_destroy(tls);else if(fd>=0)::close(fd);}};
 std::shared_ptr<Socket> socket;
 const char *trust=nullptr;
 unsigned handshakeMs=6000;
 bool secure=false,insecure=false;
 int readData(uint8_t *out,size_t n,int flags=0){if(!socket||!socket->live)return -1;int rc=socket->tls?esp_tls_conn_read(socket->tls,out,n):recv(socket->fd,out,n,flags|MSG_DONTWAIT);if(rc==0)socket->live=false;if(rc<0&&rc!=ESP_TLS_ERR_SSL_WANT_READ&&rc!=ESP_TLS_ERR_SSL_WANT_WRITE&&(socket->tls||(errno!=EAGAIN&&errno!=EWOULDBLOCK)))socket->live=false;return rc;}
public:
 WiFiClient()=default;
 explicit WiFiClient(int fd){socket=std::make_shared<Socket>();socket->fd=fd;socket->live=true;fcntl(fd,F_SETFL,O_NONBLOCK);}
 explicit operator bool()const{return socket&&socket->live;}
 bool operator==(const WiFiClient &other)const{return socket==other.socket;}
 bool operator!=(const WiFiClient &other)const{return !(*this==other);}
 void setCACert(const char *ca){trust=ca;secure=true;insecure=false;}
 void setInsecure(){secure=true;insecure=true;trust=nullptr;}
 // Socket deadlines use seconds; generic Stream deadlines use milliseconds.
 void setTimeout(unsigned seconds){Stream::setTimeout(xtouch_idf::socket_timeout_ms(seconds));}
 void setTimeoutMs(unsigned milliseconds){Stream::setTimeout(milliseconds);}
 void setHandshakeTimeout(unsigned seconds){handshakeMs=seconds*1000;}
 virtual int connect(const char *host,uint16_t port,int32_t timeout=2000);
 int connect(IPAddress address,uint16_t port,int32_t timeout=2000){auto text=address.toString();return connect(text.c_str(),port,timeout);}
 bool connected(){if(!socket||!socket->live)return false;if(socket->cursor<socket->count)return true;pollfd p{socket->fd,POLLIN,0};if(poll(&p,1,0)>0&&(p.revents&(POLLERR|POLLNVAL)))socket->live=false;return socket->live;}
 void stop(){if(socket){socket->live=false;if(socket->fd>=0)shutdown(socket->fd,SHUT_RDWR);}socket.reset();}
 int available()override{
  if(!connected())return 0;
  return xtouch_idf::receive_available(socket->received,sizeof(socket->received),socket->cursor,socket->count,
   [this](uint8_t *p,size_t size){return readData(p,size);});
 }

 int read()override{uint8_t c;return read(&c,1)==1?c:-1;}
 int read(uint8_t *out,size_t n){
  if(!socket||!n)return 0;
  if(socket->cursor==socket->count){
   socket->cursor=socket->count=0;
   if(n>=sizeof(socket->received))return readData(out,n);
   int count=readData(socket->received,sizeof(socket->received));if(count<=0)return count;socket->count=count;
  }
  size_t count=std::min(n,socket->count-socket->cursor);memcpy(out,socket->received+socket->cursor,count);socket->cursor+=count;return count;
 }
 int read(char *out,size_t n){return read((uint8_t*)out,n);}
 int peek()override{if(!socket)return -1;if(socket->cursor<socket->count)return socket->received[socket->cursor];int count=readData(socket->received,sizeof(socket->received));if(count<=0)return -1;socket->cursor=0;socket->count=count;return socket->received[0];}
 using Print::write;
 size_t write(uint8_t c)override{return write(&c,1);}
 size_t write(const uint8_t *p,size_t n)override;
 size_t write_P(const char *p,size_t n){return write((const uint8_t*)p,n);}
 template<class Source>size_t write(Source &source){size_t n=0;uint8_t buffer[1024];while(source.available()){size_t count=source.readBytes(buffer,std::min(sizeof(buffer),size_t(source.available())));if(!count)break;size_t written=write(buffer,count);n+=written;if(written!=count)break;}return n;}
 void flush()override{}
 void setNoDelay(bool enabled){if(socket){int value=enabled;setsockopt(socket->fd,IPPROTO_TCP,TCP_NODELAY,&value,sizeof(value));}}
 int fd()const{return socket?socket->fd:-1;}
 IPAddress localIP()const{sockaddr_in addr{};socklen_t n=sizeof(addr);if(socket)getsockname(socket->fd,(sockaddr*)&addr,&n);return IPAddress(addr.sin_addr.s_addr);}
 IPAddress remoteIP()const{sockaddr_in addr{};socklen_t n=sizeof(addr);if(socket)getpeername(socket->fd,(sockaddr*)&addr,&n);return IPAddress(addr.sin_addr.s_addr);}
};
class WiFiClientSecure:public WiFiClient {public:WiFiClientSecure(){secure=true;}};
class WiFiServer {
 int listener=-1;IPAddress address;uint16_t port;
public:
 explicit WiFiServer(uint16_t number=80):port(number){}
 WiFiServer(IPAddress ip,uint16_t number):address(ip),port(number){}
 ~WiFiServer(){end();}
 void begin(uint16_t number=0){if(number)port=number;if(listener>=0)return;listener=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener<0){printf("[HTTP] listener socket failed errno=%d\n",errno);return;}int yes=1;setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(port);addr.sin_addr.s_addr=uint32_t(address);if(bind(listener,(sockaddr*)&addr,sizeof(addr))||listen(listener,3)){printf("[HTTP] listener port=%u failed errno=%d\n",port,errno);end();return;}fcntl(listener,F_SETFL,O_NONBLOCK);printf("[HTTP] listener port=%u fd=%d ready\n",port,listener);}
 void end(){if(listener>=0){::close(listener);listener=-1;}}
 void close(){end();}
 void stop(){end();}
 WiFiClient available(){int fd=::accept(listener,nullptr,nullptr);return fd>=0?WiFiClient(fd):WiFiClient();}
 WiFiClient accept(){return available();}
 bool hasClient(){pollfd p{listener,POLLIN,0};return listener>=0&&poll(&p,1,0)>0&&(p.revents&POLLIN);}
 void setNoDelay(bool){}
};

#endif
