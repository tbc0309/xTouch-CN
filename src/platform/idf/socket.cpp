#include "socket.h"
int WiFiClient::connect(const char *host,uint16_t port,int32_t timeout){
 stop();if(!host||!*host)return 0;socket=std::make_shared<Socket>();
 if(secure){
  if(!trust&&!insecure){socket.reset();return 0;}
  esp_tls_cfg_t cfg{};cfg.timeout_ms=std::max(timeout,int32_t(handshakeMs));cfg.cacert_buf=(const unsigned char*)trust;cfg.cacert_bytes=trust?strlen(trust)+1:0;cfg.skip_common_name=!trust;
  socket->tls=esp_tls_init();if(!socket->tls){socket.reset();return 0;}
  if(esp_tls_conn_new_sync(host,strlen(host),port,&cfg,socket->tls)!=1){socket.reset();return 0;}
  if(esp_tls_get_conn_sockfd(socket->tls,&socket->fd)!=ESP_OK){socket.reset();return 0;}
 }else{
  addrinfo hints{};hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_INET;addrinfo *list=nullptr;char service[8];snprintf(service,sizeof(service),"%u",port);
  if(getaddrinfo(host,service,&hints,&list)||!list){socket.reset();return 0;}
  int fd=::socket(list->ai_family,list->ai_socktype,list->ai_protocol);if(fd<0){freeaddrinfo(list);socket.reset();return 0;}
  socket->fd=fd;fcntl(fd,F_SETFL,O_NONBLOCK);int rc=::connect(fd,list->ai_addr,list->ai_addrlen);freeaddrinfo(list);
  if(rc<0&&errno!=EINPROGRESS){socket.reset();return 0;}
  if(rc<0){pollfd p{fd,POLLOUT,0};int error=0;socklen_t n=sizeof(error);if(poll(&p,1,timeout)<=0||getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&n)||error){socket.reset();return 0;}}
 }
 fcntl(socket->fd,F_SETFL,O_NONBLOCK);socket->live=true;return 1;
}
size_t WiFiClient::write(const uint8_t *p,size_t n){
 if(!connected())return 0;size_t used=0;uint32_t start=millis();
 while(used<n&&millis()-start<timeoutMs){int rc=socket->tls?esp_tls_conn_write(socket->tls,p+used,n-used):send(socket->fd,p+used,n-used,MSG_DONTWAIT);if(rc>0){used+=rc;start=millis();}else if(rc==ESP_TLS_ERR_SSL_WANT_READ||rc==ESP_TLS_ERR_SSL_WANT_WRITE||errno==EAGAIN||errno==EWOULDBLOCK)delay(1);else{socket->live=false;break;}}
 return used;
}
