#include "features/config.h"
#if XTOUCH_FEATURE_CAMERA
#include "camera_rtsp.h"
#include "camera_rtp.h"
#include "camera_protocol.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <esp_h264_dec_sw.h>
#include <esp_h264_dec_param.h>
#include <esp_random.h>
#include <mbedtls/base64.h>
#include <esp_rom_md5.h>
#include <string>
#include <algorithm>
#include <functional>

namespace {
using std::string;
string lower(string s){for(char &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
string field(const string &s,const char *name){string key=lower(name);size_t i=0;while(i<s.size()){size_t e=s.find("\r\n",i);if(e==string::npos)e=s.size();size_t c=s.find(':',i);if(c<e&&lower(s.substr(i,c-i))==key){size_t v=s.find_first_not_of(" \t",c+1);return v<e?s.substr(v,e-v):"";}i=e+2;}return "";}
string parameter(const string &s,const char *key){size_t p=lower(s).find(string(key)+"=");if(p==string::npos)return "";p+=strlen(key)+1;if(p>=s.size())return "";bool quote=s[p]=='"';if(quote)++p;size_t end=s.find(quote?'"':',',p);return s.substr(p,end==string::npos?end:end-p);}
string md5(const string &s){unsigned char digest[16];md5_context_t context;esp_rom_md5_init(&context);esp_rom_md5_update(&context,s.data(),s.size());esp_rom_md5_final(digest,&context);char text[33];for(int i=0;i<16;++i)snprintf(text+2*i,3,"%02x",digest[i]);return text;}
bool safe(const string &s){return s.size()<512&&s.find_first_of("\r\n")==string::npos;}
bool printerUrl(const string &url,const char *host){for(const char *scheme:{"rtsps://","rtsp://"})for(const char *port:{":322/","/"})if(url.compare(0,strlen(scheme)+strlen(host)+strlen(port),string(scheme)+host+port)==0)return true;return false;}
struct Session {
 WiFiClientSecure client;std::atomic<bool> &stop;string root,session,challenge,code;unsigned cseq=0,nc=0;int status=0;uint8_t *packet;size_t packetSize=0;unsigned channel=0,payload=96;std::function<bool()> interleaved;
 Session(const char *host,const char *password,std::atomic<bool> &cancel,uint8_t *buffer):stop(cancel),root(string("rtsps://")+host+":322/streaming/live/1"),code(password),packet(buffer){}
 ~Session(){client.stop();std::fill(code.begin(),code.end(),0);}
 bool read(uint8_t *p,size_t n){uint32_t began=millis();bool ok=CameraProtocol::receive(client,p,n,[]{return uint32_t(millis());},[]{vTaskDelay(1);},[this]{return bool(stop);});if(!ok&&!stop)printf("[CAMERA] RTSPS read failed: bytes=%u elapsed=%u ms connected=%u\n",unsigned(n),unsigned(millis()-began),unsigned(client.connected()));return ok;}
 bool block(){uint8_t h[3];if(!read(h,3))return false;packetSize=(unsigned(h[1])<<8)|h[2];if(!packetSize)return false;if(!read(packet,packetSize))return false;channel=h[0];return true;}
 string authorization(const char *method,const string &url){
  if(challenge.empty())return "";
  if(lower(challenge).find("basic")==0){unsigned char out[96];size_t n=0;string cred="bblp:"+code;if(mbedtls_base64_encode(out,sizeof(out),&n,(const unsigned char*)cred.data(),cred.size()))return "";return "Authorization: Basic "+string((char*)out,n)+"\r\n";}
  if(lower(challenge).find("digest")!=0)return "";
  string realm=parameter(challenge,"realm"),nonce=parameter(challenge,"nonce"),qop=parameter(challenge,"qop"),algorithm=lower(parameter(challenge,"algorithm")),opaque=parameter(challenge,"opaque");
  if(nonce.empty()||!safe(nonce)||!safe(realm)||!safe(opaque)||(!algorithm.empty()&&algorithm!="md5"))return "";
  if(!qop.empty()){string modes=","+qop+",";modes.erase(std::remove(modes.begin(),modes.end(),' '),modes.end());if(modes.find(",auth,")==string::npos)return "";}
  char count[9];snprintf(count,sizeof(count),"%08x",++nc);char random[17];snprintf(random,sizeof(random),"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());string cn=random;
  string response=md5(md5("bblp:"+realm+":"+code)+":"+nonce+":"+(qop.empty()?"":string(count)+":"+cn+":auth:")+md5(string(method)+":"+url));
  string a="Authorization: Digest username=\"bblp\", realm=\""+realm+"\", nonce=\""+nonce+"\", uri=\""+url+"\", response=\""+response+"\"";
  if(!qop.empty())a+=", qop=auth, nc="+string(count)+", cnonce=\""+cn+"\"";
  if(!opaque.empty())a+=", opaque=\""+opaque+"\"";
  return a+"\r\n";
 }
 bool request(const char *method,const string &url,const string &extra,string &headers,string &body){
  if(!safe(url)||!safe(session))return false;
  for(unsigned attempt=0;attempt<2&&!stop;++attempt){
   status=0;
   unsigned seq=++cseq;string req=string(method)+" "+url+" RTSP/1.0\r\nCSeq: "+std::to_string(seq)+"\r\nUser-Agent: xTouch-CN\r\n"+authorization(method,url)+(session.empty()?"":"Session: "+session+"\r\n")+extra+"\r\n";
   if(client.write((const uint8_t*)req.data(),req.size())!=req.size())return false;
   headers.clear();body.clear();uint8_t c;
   uint32_t began=millis();while(!stop){if(uint32_t(millis()-began)>6000||!read(&c,1))return false;if(headers.empty()&&c=='$'){if(!block()||(interleaved&&!interleaved()))return false;continue;}headers+=char(c);if(headers.size()>8192)return false;if(headers.size()>=4&&headers.compare(headers.size()-4,4,"\r\n\r\n")==0)break;}
   if(stop||sscanf(headers.c_str(),"RTSP/1.0 %d",&status)!=1)return false;
   string cs=field(headers,"cseq");if(cs!=std::to_string(seq))return false;
   string len=field(headers,"content-length");char *end=nullptr;unsigned long n=len.empty()?0:strtoul(len.c_str(),&end,10);if(n>4096||(!len.empty()&&(!end||*end)))return false;
   body.resize(n);if(n&&!read((uint8_t*)&body[0],n))return false;
   if(status!=401)return status>=200&&status<300;
   string next=field(headers,"www-authenticate");if(parameter(next,"nonce")!=parameter(challenge,"nonce"))nc=0;challenge=next;if(challenge.empty()||challenge.size()>1024)return false;
  }return false;
 }
};
struct Decoder {
 esp_h264_dec_handle_t handle=nullptr;CameraRtp::Sps info;const CameraRtspSink &sink;std::atomic<bool> &stop;uint8_t *nal;bool configured=false,keyFrame=false;int error=0;uint32_t started=0,published=0;
 ~Decoder(){if(handle){esp_h264_dec_close(handle);esp_h264_dec_del(handle);}}
 bool emit(const uint8_t *p,size_t n){
  if(stop)return false;
  unsigned type=p[0]&31;if(type==9||type==6||type==12)return true;
  if(type==8&&!CameraRtp::pps(p,n)){error=7;return false;}
  if(type==7){CameraRtp::Sps next;if(!CameraRtp::sps(p,n,next)){error=7;return false;}
   if(configured&&(next.width!=info.width||next.height!=info.height||next.refs!=info.refs||next.visibleWidth!=info.visibleWidth||next.visibleHeight!=info.visibleHeight)){error=3;return false;}
   if(!configured){size_t need=CameraRtp::decoderBytes(next);if(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<need+CameraFrame::psramReserve||heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<80000){error=5;return false;}info=next;configured=true;}
  }
  if(!configured)return true;
  if(type==1&&!keyFrame)return true; // PLAY may join between keyframes.
  if(type==5)keyFrame=true;
  if(!handle){esp_h264_dec_cfg_sw_t cfg{};cfg.pic_type=ESP_H264_RAW_FMT_I420;if(esp_h264_dec_sw_new(&cfg,&handle)!=ESP_H264_ERR_OK||esp_h264_dec_open(handle)!=ESP_H264_ERR_OK){error=5;return false;}}
  if(n>CameraProtocol::maxFrame-4){error=3;return false;}memcpy(nal+4,p,n);nal[0]=nal[1]=nal[2]=0;nal[3]=1;
  esp_h264_dec_in_frame_t in{};in.raw_data.buffer=nal;in.raw_data.len=n+4;started=millis();
  while(in.raw_data.len&&!stop){esp_h264_dec_out_frame_t out{};auto ret=esp_h264_dec_process(handle,&in,&out);
   if(ret!=ESP_H264_ERR_OK){error=ret==ESP_H264_ERR_MEM?5:3;return false;}
   if(uint32_t(millis()-started)>CameraFrame::decodeBudgetMs){error=11;return false;}
   if(out.out_size){size_t expected=size_t(info.width)*info.height*3/2;if(out.out_size!=expected||!out.outbuf){error=3;return false;}
    if(!CameraRtp::i420(out.outbuf,info.width,info.height,sink.pixels,sink.width,sink.height,[this]{return stop||uint32_t(millis()-started)>CameraFrame::decodeBudgetMs;},[]{vTaskDelay(1);},info.visibleWidth,info.visibleHeight)){error=11;return false;}
    sink.publish(info.visibleWidth,info.visibleHeight);published=millis();sink.status(6);
   }
   if(!in.consume||in.consume>in.raw_data.len){error=3;return false;}in.raw_data.buffer+=in.consume;in.raw_data.len-=in.consume;vTaskDelay(1);
  }return !stop;
 }
};
}
int cameraRtspRun(const char *host,const char *code,std::atomic<bool> &stop,const CameraRtspSink &sink){
 uint8_t *packet=(uint8_t*)ps_malloc(65536),*assembly=(uint8_t*)ps_malloc(CameraProtocol::maxFrame),*nal=(uint8_t*)ps_malloc(CameraProtocol::maxFrame);
 if(!packet||!assembly||!nal){free(packet);free(assembly);free(nal);return 5;}
 int failure=1;
 {
 Session s(host,code,stop,packet);Decoder decoder{nullptr,{},sink,stop,nal};CameraRtp::Assembler rtp{assembly,CameraProtocol::maxFrame-4};
 s.client.setInsecure();s.client.setTimeout(250);s.client.setHandshakeTimeout(3);
 if(s.client.connect(host,322,3000)){
  string headers,body,track,base;bool setup=false;
  do {
   if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<80000){failure=5;break;}
   if(!s.request("DESCRIBE",s.root,"Accept: application/sdp\r\n",headers,body)){failure=s.status==401?12:s.status?1:2;break;}
   base=field(headers,"content-base");if(base.empty())base=s.root+"/";
   // Only use URLs on this printer. Never follow an SDP URL to another host.
   if(!printerUrl(base,host)){failure=3;break;}
   bool video=false,h264=false;string params;size_t pos=0;
   while(pos<body.size()){size_t end=body.find('\n',pos);if(end==string::npos)end=body.size();string line=body.substr(pos,end-pos);if(!line.empty()&&line.back()=='\r')line.pop_back();pos=end+1;
    if(line.compare(0,2,"m=")==0){if(video)break;video=line.compare(0,8,"m=video ")==0;}
    if(!video)continue;
    unsigned pt=0;if(sscanf(line.c_str(),"a=rtpmap:%u H264/90000",&pt)==1&&line.find("H264/90000")!=string::npos&&pt<128){s.payload=pt;h264=true;}
    if(line.compare(0,10,"a=control:")==0)track=line.substr(10);
    if(line.compare(0,7,"a=fmtp:")==0)params=line;
   }
   if(!h264||track.empty()||!safe(track)||params.find("packetization-mode=2")!=string::npos){failure=7;break;}
   string url=track.find("://")!=string::npos?track:base+(base.back()=='/'?"":"/")+track;
   if(!printerUrl(url,host)){failure=3;break;}
   bool paramsValid=true;size_t sp=params.find("sprop-parameter-sets=");
   if(sp!=string::npos){string sets=params.substr(sp+21);size_t semi=sets.find(';');if(semi!=string::npos)sets.resize(semi);size_t comma=sets.find(',');
    if(comma==string::npos)paramsValid=false;
    else for(auto enc:{sets.substr(0,comma),sets.substr(comma+1)}){size_t size=0;uint8_t bytes[512];if(mbedtls_base64_decode(bytes,sizeof(bytes),&size,(const unsigned char*)enc.data(),enc.size())||!size||!decoder.emit(bytes,size)){paramsValid=false;break;}}
   }
   if(!paramsValid){failure=decoder.error?decoder.error:3;break;}
   if(!s.request("SETUP",url,"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n",headers,body)){failure=s.status==401?12:1;break;}
   s.session=field(headers,"session");size_t semi=s.session.find(';');if(semi!=string::npos)s.session.resize(semi);
   if(s.session.empty()||s.session.size()>128||field(headers,"transport").find("interleaved=0-1")==string::npos){failure=3;break;}
   setup=true;if(!s.request("PLAY",s.root,"Range: npt=0.000-\r\n",headers,body)){failure=s.status==401?12:1;break;}
   uint32_t keep=millis();decoder.published=keep;unsigned malformed=0;
   s.interleaved=[&]{if(s.channel!=0)return true;bool ok=rtp.packet(s.packet,s.packetSize,s.payload,[&](const uint8_t *p,size_t n){return decoder.emit(p,n);});if(!ok)++malformed;else malformed=0;return !decoder.error&&malformed<=32;};
   while(!stop){uint8_t marker;if(!s.read(&marker,1)||marker!='$'||!s.block()){failure=2;break;}
    if(!s.interleaved()){failure=decoder.error?decoder.error:3;break;}
    if(uint32_t(millis()-keep)>15000){if(!s.request("OPTIONS",s.root,"",headers,body)){failure=decoder.error?decoder.error:2;break;}keep=millis();}
    if(uint32_t(millis()-decoder.published)>30000){failure=2;break;}
    if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<32768){printf("[CAMERA] RTSPS internal reserve: free=%u\n",unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)));failure=5;break;}vTaskDelay(1);
   }
  }while(false);
  // Closing TLS immediately also tears down the session; no blocking RTSP wait on exit.
  (void)setup;
 }
 }
 if(!stop)printf("[CAMERA] RTSPS ended: result=%d, uptime=%u ms\n",failure,unsigned(millis()));
 free(packet);free(assembly);free(nal);return failure;
}

#endif
