#pragma once
#include <Arduino.h>
#include <Stream.h>
#if defined(XTOUCH_WAVESHARE)
#include <esp_http_client.h>
#include <algorithm>

// Streaming GET transport. IDF owns TLS and decodes chunk framing; callers keep
// their existing body limits. Redirects are opt-in and never carry credentials.
class XTouchHttpClient : public Stream {
    esp_http_client_handle_t handle=nullptr;
    String url,location,authorization,userAgent,accept;
    const char *ca=nullptr;
    int timeout=2000,connectTimeout=6000,status=0;int64_t length=-1;
    bool redirects=false,done=true,failed=false;unsigned redirectLimit=5;
    uint8_t buffer[1024];size_t cursor=0,count=0;
    int fill(){
        if(cursor<count)return int(count-cursor);
        if(done||!handle)return 0;
        int n=esp_http_client_read(handle,reinterpret_cast<char*>(buffer),sizeof(buffer));
        cursor=0;count=n>0?size_t(n):0;
        if(n<=0){done=true;failed=n<0||!esp_http_client_is_complete_data_received(handle);}
        return int(count);
    }
public:
    ~XTouchHttpClient(){end();}
    XTouchHttpClient()=default;
    XTouchHttpClient(const XTouchHttpClient&)=delete;
    void setConnectTimeout(int ms){connectTimeout=ms;}
    void setTimeout(int ms){timeout=ms;Stream::setTimeout(ms);}
    void useHTTP10(bool){}
    void setFollowRedirects(bool enabled){redirects=enabled;}
    void setRedirectLimit(unsigned n){redirectLimit=n;}
    bool begin(const String &address,const char *trust=nullptr){end();url=address;ca=trust;return url.startsWith("https://")?(ca&&*ca):url.startsWith("http://");}
    void addHeader(const char *name,const String &value){
        if(!strcasecmp(name,"Authorization"))authorization=value;
        else if(!strcasecmp(name,"User-Agent"))userAgent=value;
        else if(!strcasecmp(name,"Accept"))accept=value;
    }
    int GET(){
        for(unsigned hop=0;hop<=redirectLimit;++hop){
            end();location="";
            esp_http_client_config_t config={};config.url=url.c_str();config.cert_pem=ca;
            config.timeout_ms=connectTimeout;config.disable_auto_redirect=true;config.keep_alive_enable=false;
            config.buffer_size=1024;config.buffer_size_tx=1024;config.user_data=this;
            config.event_handler=[](esp_http_client_event_t *event)->esp_err_t{
                auto *self=static_cast<XTouchHttpClient*>(event->user_data);
                if(event->event_id==HTTP_EVENT_ON_HEADER&&event->header_key&&event->header_value&&!strcasecmp(event->header_key,"Location"))self->location=event->header_value;
                return ESP_OK;
            };
            handle=esp_http_client_init(&config);if(!handle)return -1;
            if(authorization.length())esp_http_client_set_header(handle,"Authorization",authorization.c_str());
            if(userAgent.length())esp_http_client_set_header(handle,"User-Agent",userAgent.c_str());
            if(accept.length())esp_http_client_set_header(handle,"Accept",accept.c_str());
            esp_err_t rc=esp_http_client_open(handle,0);if(rc!=ESP_OK){end();return -int(rc);}
            length=esp_http_client_fetch_headers(handle);status=esp_http_client_get_status_code(handle);
            if(length<0){end();return -1;}
            if(status==301||status==302||status==303||status==307||status==308){
                if(!redirects||authorization.length()||hop==redirectLimit||!location.startsWith("https://"))return status;
                url=location;continue;
            }
            esp_http_client_set_timeout_ms(handle,timeout);failed=false;done=false;return status;
        }
        return -1;
    }
    int getSize()const{return length>INT_MAX?-1:(length==0&&handle&&esp_http_client_is_chunked_response(handle)?-1:int(length));}
    Stream *getStreamPtr(){return this;}
    bool connected()const{return handle&&!done;}
    int available()override{return fill();}
    int read()override{return fill()?buffer[cursor++]:-1;}
    int peek()override{return fill()?buffer[cursor]:-1;}
    size_t readBytes(uint8_t *dest,size_t wanted){size_t n=0;while(n<wanted&&fill()){size_t take=std::min(wanted-n,count-cursor);memcpy(dest+n,buffer+cursor,take);cursor+=take;n+=take;}return n;}
    int writeToStream(Stream *sink){int total=0;while(fill()){size_t n=count-cursor;if(sink->write(buffer+cursor,n)!=n)return -1;total+=n;cursor=count;}return failed?-1:total;}
    size_t write(uint8_t)override{return 0;}
    void flush()override{}
    void end(){if(handle){esp_http_client_close(handle);esp_http_client_cleanup(handle);handle=nullptr;}done=true;cursor=count=0;}
};
#else
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
class XTouchHttpClient : public HTTPClient {
    WiFiClientSecure tls;WiFiClient plain;
public:
    bool begin(const String &url,const char *ca=nullptr){if(url.startsWith("https://")){if(!ca)return false;tls.setCACert(ca);tls.setHandshakeTimeout(6);return HTTPClient::begin(tls,url);}return HTTPClient::begin(plain,url);}
    void setFollowRedirects(bool enabled){HTTPClient::setFollowRedirects(enabled?HTTPC_STRICT_FOLLOW_REDIRECTS:HTTPC_DISABLE_FOLLOW_REDIRECTS);}
};
#endif
