#pragma once
#include <WiFi.h>
#include <esp_system.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>
#include <lwip/sockets.h>
#include <errno.h>
#include <algorithm>
#include <cstring>

/* FTPS-specific transport: bounded polling, TLS 1.2 and data-session reuse.
 * No Stream::setTimeout ambiguity, blocking TLS reads, or shared MQTT socket. */
class PreviewTransport {
 WiFiClient tcp;
 mbedtls_ssl_context ssl;
 mbedtls_ssl_config config;
 bool (*cancel)();
 bool live=false,clear=false;
 uint32_t handshakeMs=4000,ioMs=2000;
 int error=0;unsigned phase=0;
 uint8_t buffered[512];size_t at=0,count=0;
 static int send(void *ctx,const unsigned char *data,size_t n){
  auto *self=(PreviewTransport*)ctx;if(self->cancel())return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
  int written=::send(self->tcp.fd(),data,n,MSG_DONTWAIT);
  if(written>=0)return written;
  return errno==EAGAIN||errno==EWOULDBLOCK?MBEDTLS_ERR_SSL_WANT_WRITE:MBEDTLS_ERR_SSL_INTERNAL_ERROR;
 }
 static int receive(void *ctx,unsigned char *data,size_t n){
  auto *self=(PreviewTransport*)ctx;if(self->cancel())return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
  int available=self->tcp.available();
  if(!available)return self->tcp.connected()?MBEDTLS_ERR_SSL_WANT_READ:0;
  int read=self->tcp.read(data,std::min(n,size_t(available)));
  return read>0?read:MBEDTLS_ERR_SSL_WANT_READ;
 }
 bool waiting(int rc){return rc==MBEDTLS_ERR_SSL_WANT_READ||rc==MBEDTLS_ERR_SSL_WANT_WRITE;}
public:
 explicit PreviewTransport(bool (*cancelled)()):cancel(cancelled){mbedtls_ssl_init(&ssl);mbedtls_ssl_config_init(&config);}
 PreviewTransport(const PreviewTransport&)=delete;
 ~PreviewTransport(){stop();}
 void setHandshakeTimeoutMs(uint32_t ms){handshakeMs=ms;}
 void setIoTimeoutMs(uint32_t ms){ioMs=ms;}
 bool connect(const char *host,uint16_t port,int32_t timeoutMs,PreviewTransport *control=nullptr,bool plaintext=false){
  stop();error=0;phase=1;clear=plaintext;
  if(cancel()||!tcp.connect(host,port,timeoutMs))return false;
  tcp.setTimeout(1); // Arduino TCP API takes seconds; send bound is one second.
  if(clear){live=true;return true;}
  if(psa_crypto_init()!=PSA_SUCCESS)return false;
  phase=2;error=mbedtls_ssl_config_defaults(&config,MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT);
  if(error)return false;
  mbedtls_ssl_conf_authmode(&config,MBEDTLS_SSL_VERIFY_NONE); // Printer LAN FTPS uses a self-signed certificate.
  mbedtls_ssl_conf_min_tls_version(&config,MBEDTLS_SSL_VERSION_TLS1_2);
  mbedtls_ssl_conf_max_tls_version(&config,MBEDTLS_SSL_VERSION_TLS1_2);
  phase=4;error=mbedtls_ssl_setup(&ssl,&config);if(error)return false;
  phase=5;error=mbedtls_ssl_set_hostname(&ssl,host);if(error)return false;
  mbedtls_ssl_set_bio(&ssl,this,send,receive,nullptr);
  if(control){
   mbedtls_ssl_session session;mbedtls_ssl_session_init(&session);
   error=mbedtls_ssl_get_session(&control->ssl,&session);
   if(!error)error=mbedtls_ssl_set_session(&ssl,&session);
   mbedtls_ssl_session_free(&session);if(error)return false;
  }
  phase=6;uint32_t start=millis();
  while(!cancel()&&uint32_t(millis()-start)<handshakeMs){
   error=mbedtls_ssl_handshake(&ssl);if(!error){live=true;return true;}
   if(!waiting(error))return false;delay(1);
  }
  error=MBEDTLS_ERR_SSL_TIMEOUT;return false;
 }
 size_t write(const uint8_t *bytes,size_t size){
  if(!live)return 0;size_t done=0;uint32_t start=millis();
  while(done<size&&!cancel()&&uint32_t(millis()-start)<ioMs){
   int rc=clear?send(this,bytes+done,size-done):mbedtls_ssl_write(&ssl,bytes+done,size-done);
   if(rc>0)done+=rc;else if(!waiting(rc)){error=rc;live=false;break;}else delay(1);
  }return done;
 }
 size_t print(const String &text){return write((const uint8_t*)text.c_str(),text.length());}
 int available(){
  if(at<count)return count-at;
  at=count=0;if(!live||cancel())return 0;
  if(!tcp.available()&&(clear||mbedtls_ssl_get_bytes_avail(&ssl)==0))return 0;
  int rc=clear?tcp.read(buffered,sizeof(buffered)):mbedtls_ssl_read(&ssl,buffered,sizeof(buffered));
  if(rc>0){count=rc;return count;}
  if(!waiting(rc)){error=rc;live=false;}return 0;
 }
 int read(uint8_t *out,size_t n){int ready=available();if(!ready)return 0;size_t take=std::min(n,size_t(ready));memcpy(out,buffered+at,take);at+=take;return take;}
 int read(){uint8_t c;return read(&c,1)==1?c:-1;}
 int peek(){return available()?buffered[at]:-1;}
 bool connected(){return live&&(at<count||tcp.connected());}
 unsigned lastPhase()const{return phase;}
 int lastError(char*,size_t){return error;}
 void stop(){if(live&&!clear&&!cancel())mbedtls_ssl_close_notify(&ssl);tcp.stop();live=false;at=count=0;mbedtls_ssl_free(&ssl);mbedtls_ssl_config_free(&config);mbedtls_ssl_init(&ssl);mbedtls_ssl_config_init(&config);}
};

