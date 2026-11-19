#pragma once
#include "platform/idf/socket.h"
#include <esp_system.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>
#include <lwip/sockets.h>
#include <errno.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
#include <esp_heap_caps.h>

/* FTPS-specific transport: bounded polling, TLS 1.2 and data-session reuse.
 * No Stream::setTimeout ambiguity, blocking TLS reads, or shared MQTT socket. */
class PreviewTransport {
 WiFiClient tcp;
 struct TlsState {
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config config;
  TlsState(){mbedtls_ssl_init(&ssl);mbedtls_ssl_config_init(&config);}
  ~TlsState(){mbedtls_ssl_free(&ssl);mbedtls_ssl_config_free(&config);}
 };
 struct TlsDeleter {void operator()(TlsState *p)const{if(p){p->~TlsState();heap_caps_free(p);}}};
 std::unique_ptr<TlsState,TlsDeleter> tls;
 bool (*cancel)();
 bool live=false,clear=false,notifySent=false,peerClosed=false;
 unsigned discarded=0;
 uint32_t handshakeMs=4000,ioMs=2000,cleanupStarted=0,cleanupMs=0;
 bool cleanup=false;
 bool interrupted()const{return cleanup?uint32_t(millis()-cleanupStarted)>=cleanupMs:cancel();}
 int error=0;unsigned phase=0;
 uint8_t buffered[512];size_t at=0,count=0;
 static int send(void *ctx,const unsigned char *data,size_t n){
  auto *self=(PreviewTransport*)ctx;if(self->interrupted())return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
  int written=::send(self->tcp.fd(),data,n,MSG_DONTWAIT);
  if(written>=0)return written;
  return errno==EAGAIN||errno==EWOULDBLOCK?MBEDTLS_ERR_SSL_WANT_WRITE:MBEDTLS_ERR_SSL_INTERNAL_ERROR;
 }
 static int receive(void *ctx,unsigned char *data,size_t n){
  auto *self=(PreviewTransport*)ctx;if(self->interrupted())return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
  int available=self->tcp.available();
  if(!available)return self->tcp.connected()?MBEDTLS_ERR_SSL_WANT_READ:0;
  int read=self->tcp.read(data,std::min(n,size_t(available)));
  return read>0?read:MBEDTLS_ERR_SSL_WANT_READ;
 }
 bool waiting(int rc){return rc==MBEDTLS_ERR_SSL_WANT_READ||rc==MBEDTLS_ERR_SSL_WANT_WRITE;}
public:
 explicit PreviewTransport(bool (*cancelled)()):cancel(cancelled){}
 PreviewTransport(const PreviewTransport&)=delete;
 ~PreviewTransport(){stop();}
 void setHandshakeTimeoutMs(uint32_t ms){handshakeMs=ms;}
 void setIoTimeoutMs(uint32_t ms){ioMs=ms;}
 void beginCleanup(uint32_t ms){if(!cleanup){cleanup=true;cleanupStarted=millis();cleanupMs=ms;}ioMs=std::min(ioMs,ms);}
 bool connect(const char *host,uint16_t port,int32_t timeoutMs,PreviewTransport *control=nullptr,bool plaintext=false){
  stop();cleanup=false;notifySent=peerClosed=false;discarded=0;error=0;phase=1;clear=plaintext;
  if(interrupted()||!tcp.connect(host,port,timeoutMs))return false;
  tcp.setTimeout(1); // Arduino TCP API takes seconds; send bound is one second.
  if(clear){live=true;return true;}
  void *storage=heap_caps_malloc(sizeof(TlsState),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!storage)storage=heap_caps_malloc(sizeof(TlsState),MALLOC_CAP_8BIT);
  if(!storage){error=MBEDTLS_ERR_SSL_ALLOC_FAILED;return false;}
  tls.reset(new(storage) TlsState);
  if(psa_crypto_init()!=PSA_SUCCESS)return false;
  phase=2;error=mbedtls_ssl_config_defaults(&tls->config,MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT);
  if(error)return false;
  mbedtls_ssl_conf_authmode(&tls->config,MBEDTLS_SSL_VERIFY_NONE); // Printer LAN FTPS uses a self-signed certificate.
  mbedtls_ssl_conf_min_tls_version(&tls->config,MBEDTLS_SSL_VERSION_TLS1_2);
  mbedtls_ssl_conf_max_tls_version(&tls->config,MBEDTLS_SSL_VERSION_TLS1_2);
  phase=4;error=mbedtls_ssl_setup(&tls->ssl,&tls->config);if(error)return false;
  phase=5;error=mbedtls_ssl_set_hostname(&tls->ssl,host);if(error)return false;
  mbedtls_ssl_set_bio(&tls->ssl,this,send,receive,nullptr);
  if(control){
   if(!control->tls){error=MBEDTLS_ERR_SSL_BAD_INPUT_DATA;return false;}
   mbedtls_ssl_session session;mbedtls_ssl_session_init(&session);
   error=mbedtls_ssl_get_session(&control->tls->ssl,&session);
   if(!error)error=mbedtls_ssl_set_session(&tls->ssl,&session);
   mbedtls_ssl_session_free(&session);if(error)return false;
  }
  phase=6;uint32_t start=millis();
  while(!interrupted()&&uint32_t(millis()-start)<handshakeMs){
   error=mbedtls_ssl_handshake(&tls->ssl);if(!error){live=true;return true;}
   if(!waiting(error))return false;delay(1);
  }
  error=MBEDTLS_ERR_SSL_TIMEOUT;return false;
 }
 size_t write(const uint8_t *bytes,size_t size){
  if(!live)return 0;size_t done=0;uint32_t start=millis();
  while(done<size&&!interrupted()&&uint32_t(millis()-start)<ioMs){
   int rc=clear?send(this,bytes+done,size-done):mbedtls_ssl_write(&tls->ssl,bytes+done,size-done);
   if(rc>0)done+=rc;else if(!waiting(rc)){error=rc;live=false;break;}else delay(1);
  }return done;
 }
 size_t print(const String &text){return write((const uint8_t*)text.c_str(),text.length());}
 int available(){
  if(at<count)return count-at;
  at=count=0;if(!live||interrupted())return 0;
  if(!tcp.available()&&(clear||mbedtls_ssl_get_bytes_avail(&tls->ssl)==0))return 0;
  int rc=clear?tcp.read(buffered,sizeof(buffered)):mbedtls_ssl_read(&tls->ssl,buffered,sizeof(buffered));
  if(rc>0){count=rc;return count;}
  if(!waiting(rc)){error=rc;live=false;}return 0;
 }
 int read(uint8_t *out,size_t n){int ready=available();if(!ready)return 0;size_t take=std::min(n,size_t(ready));memcpy(out,buffered+at,take);at+=take;return take;}
 int read(){uint8_t c;return read(&c,1)==1?c:-1;}
 int peek(){return available()?buffered[at]:-1;}
 bool connected(){return live&&(at<count||tcp.connected());}
 unsigned lastPhase()const{return phase;}
 int lastError(char*,size_t){return error;}
 bool closeNotifySent()const{return notifySent;}
 bool peerCloseNotify()const{return peerClosed;}
 unsigned shutdownDiscardedBytes()const{return discarded;}
 bool isClosed()const{return !live&&!tls&&tcp.fd()<0;}
 void stop(bool waitForPeer=false){
  if(live&&!clear&&tls&&!interrupted()){
   uint32_t start=millis();
   // WANT_WRITE does not mean close_notify reached the wire.
   while(!interrupted()&&uint32_t(millis()-start)<250){
    int rc=mbedtls_ssl_close_notify(&tls->ssl);
    if(!rc){notifySent=true;break;}
    if(!waiting(rc)){error=rc;break;}
    delay(1);
   }
   if(waitForPeer&&notifySent){
    uint8_t drain[256];
    while(!interrupted()&&uint32_t(millis()-start)<1000&&discarded<32768){
     int rc=mbedtls_ssl_read(&tls->ssl,drain,std::min(sizeof(drain),size_t(32768-discarded)));
     if(rc>0){discarded+=unsigned(rc);delay(1);continue;}
     if(rc==MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY){peerClosed=true;error=rc;break;}
     if(!rc||!waiting(rc)){error=rc;break;}
     delay(1);
    }
   }
  }
  tcp.stop();live=false;at=count=0;tls.reset();
 }
};
static_assert(sizeof(PreviewTransport)<1024,"Keep TLS state off the preview task stack");
