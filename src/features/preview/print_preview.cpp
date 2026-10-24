#include "features/ui_profile.h"
#if !XTOUCH_HAS_PRINT_PREVIEW
#include "print_preview.h"
void xtouch_print_preview_poll() {}
void xtouch_print_preview_suspend(bool) {}
XTouchPrintPreviewStatus xtouch_print_preview_status() { return {}; }
#else
#include "print_preview.h"
#include "xtouch/printers.h"
#include "preview_source.h"
#include "preview_archive.h"
#include "preview_zip_stream.h"
#include "xtouch/cloud.h"
#include "xtouch/portal.h"
#include "xtouch/types.h"
#include "ui/print_preview_view.h"
#include "ui/settings_gesture.h"
#include <Arduino.h>
#include <WiFi.h>
#include "preview_transport.h"
#include <PNGdec.h>
#include <HTTPClient.h>
#include "preview_ca.h"
#include "preview_api_ca.h"
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <memory>
#include <new>
#include <algorithm>

namespace {
std::atomic<bool> running{false},complete{false},suspended{false};
std::atomic<unsigned> generation{0};
struct Job {char host[64],code[9],file[160],title[160];unsigned generation,plate;uint16_t port;bool plainData;char token[2049],serial[32],task[48];bool china;} job;
struct Image {uint16_t *pixels;unsigned w,h;Image():pixels(nullptr),w(0),h(0){}Image(uint16_t *p,unsigned x,unsigned y):pixels(p),w(x),h(y){}} ready,shown;
lv_img_dsc_t descriptor={};
String activeKey;unsigned tries=0;uint32_t nextAttempt=0,started=0;int lastResult=0;
std::atomic<unsigned> renderedLines{0},tlsPhase{0};
int publishedResult=0;std::atomic<unsigned> networkStage{0};std::atomic<int> networkStatus{0},tlsStatus{0};
bool cancelled(){return suspended.load()||generation.load()!=job.generation||xtouch_portal_maintenance()||uint32_t(millis()-started)>20000;}
void *allocate(size_t n){
 void *p=heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 return p?p:heap_caps_malloc(n,MALLOC_CAP_8BIT);
}
struct Ftps {
 PreviewTransport control{cancelled};
 ~Ftps(){if(control.connected()&&!cancelled())control.print("QUIT\r\n");}
 bool readLine(String &line){
  line="";uint32_t at=millis();
  while(!cancelled()&&uint32_t(millis()-at)<4000){
   while(control.available()) {int c=control.read();if(c=='\n')return true;if(c!='\r'){if(line.length()>=384)return false;line+=(char)c;}}
   if(!control.connected())return false;delay(1);
  }return false;
 }
 int reply(String *text=nullptr){
  String line;for(unsigned i=0;i<48&&!cancelled();++i){
   if(!readLine(line))return -1;
   if(line.length()>=4&&line[0]>='1'&&line[0]<='5'&&line[3]==' '){if(text)*text=line;return line.substring(0,3).toInt();}
  }return -1;
 }
 int command(const String &line,String *text=nullptr){
  if(cancelled()||!control.connected())return -1;
  // Some printer FTP servers parse each TLS record as a complete command.
  // Keep the command and its terminator in one write (including PASS).
  control.print(line+"\r\n");return reply(text);
 }
 bool connect(){
  control.setHandshakeTimeoutMs(4000);control.setIoTimeoutMs(2000);
  if(cancelled()||!control.connect(job.host,job.port,4000)){Serial.printf("[PREVIEW] FTPS stage=tls heap=%u\n",unsigned(ESP.getFreeHeap()));return false;}
  int greeting=reply();if(greeting!=220){Serial.printf("[PREVIEW] FTPS stage=greeting rc=%d\n",greeting);return false;}
  int rc=command("USER bblp");if(rc==331)rc=command(String("PASS ")+job.code);
  if(rc!=230){Serial.printf("[PREVIEW] FTPS stage=login rc=%d\n",rc);return false;}
  for(const char *cmd:{"PBSZ 0",job.plainData?"PROT C":"PROT P","TYPE I"}){rc=command(cmd);if(rc!=200){Serial.printf("[PREVIEW] FTPS stage=%s rc=%d\n",cmd,rc);return false;}}
  return true;
 }
 bool size(const String &path,uint32_t &value){String s;if(command("SIZE "+path,&s)!=213)return false;long n=s.substring(4).toInt();if(n<=0||n>0x7fffffffL)return false;value=n;return true;}

};
// Read only the bounded ZIP prefix required for the exact task and plate.
int fetchPng(uint8_t *&png,size_t &pngSize){
 String file=job.file;
 if(file.indexOf('\r')>=0||file.indexOf('\n')>=0||file.indexOf("..")>=0)return 25;
 // Use an explicitly supplied image path only; do not guess image names from a job.
 if(!file.endsWith(".png"))return 29;
 Ftps ftp;if(!ftp.connect())return 3;uint32_t fileSize=0;
 if(!ftp.size(file,fileSize))return 25;
 const size_t cap=262144;
 if(fileSize>cap)return 11;
 png=(uint8_t*)allocate(fileSize);if(!png)return 12;
 auto fail=[&](int error){free(png);png=nullptr;return error;};
 String passive;if(ftp.command("PASV",&passive)!=227)return fail(5);
 int a,b,c,d,p1,p2,at=passive.indexOf('(');
 if(at<0||sscanf(passive.c_str()+at+1,"%d,%d,%d,%d,%d,%d",&a,&b,&c,&d,&p1,&p2)!=6||p1<0||p1>255||p2<0||p2>255||!(p1||p2))return fail(5);
 PreviewTransport data{cancelled};data.setHandshakeTimeoutMs(5000);data.setIoTimeoutMs(2000);
 ftp.control.print("RETR "+file+"\r\n");
 if(!data.connect(job.host,p1*256+p2,4000,&ftp.control,job.plainData))return fail(6);
 int reply=ftp.reply();if(reply!=150&&reply!=125)return fail(6);
 uint32_t progress=millis();
 while(pngSize<fileSize&&!cancelled()){
  int available=data.available();
  if(available>0){uint8_t scratch[512];int n=data.read(scratch,std::min(sizeof(scratch),std::min(size_t(available),size_t(fileSize-pngSize))));if(n>0)memcpy(png+pngSize,scratch,n);if(n<=0)return fail(13);pngSize+=n;progress=millis();}
  else {if(!data.connected()||uint32_t(millis()-progress)>3000)return fail(13);delay(1);}
 }
 data.stop();if(cancelled()||pngSize!=fileSize)return fail(13);
 reply=ftp.reply();if(reply!=226&&reply!=250)return fail(13);
 if(pngSize<8)return fail(19);
 const uint8_t signature[]={137,80,78,71,13,10,26,10};for(unsigned i=0;i<8;++i)if(png[i]!=signature[i])return fail(19);
 return 0;
}
int fetchArchive(uint8_t *&png,size_t &pngSize){
 Ftps ftp;if(!ftp.connect())return 3;
 String names[3];unsigned count=0;String file=job.file,title=job.title;
 auto add=[&](String name){if(!name.length()||name.indexOf("..")>=0||name.indexOf('\r')>=0||name.indexOf('\n')>=0)return;
  for(unsigned n=0;n<count;++n)if(names[n]==name)return;
  if(count<3)names[count++]=name;};
 if(file.endsWith(".3mf"))add(file);
 if(title.length()){if(title.endsWith(".3mf"))add(title);else{add(title+".gcode.3mf");add(title+".3mf");}}
 String path;uint32_t fileSize=0;unsigned matches=0;
 for(unsigned n=0;n<count;++n){
  String paths[4]={names[n],String("/cache/")+names[n],String("/model/")+names[n],String("/data/")+names[n]};
  unsigned pathsCount=names[n].indexOf('/')>=0?1:4;
  for(unsigned k=0;k<pathsCount;++k){uint32_t size=0;
   if(ftp.size(paths[k],size)){if(path==paths[k])continue;path=paths[k];fileSize=size;++matches;}
  }
 }
 // Reused task names in multiple locations are ambiguous: keep the model image.
 if(matches!=1)return 25;
 if(path.isEmpty())return 25;
 String passive;if(ftp.command("PASV",&passive)!=227)return 5;
 int a,b,c,d,p1,p2,at=passive.indexOf('(');
 if(at<0||sscanf(passive.c_str()+at+1,"%d,%d,%d,%d,%d,%d",&a,&b,&c,&d,&p1,&p2)!=6||p1<0||p1>255||p2<0||p2>255||!(p1||p2))return 5;
 PreviewTransport data{cancelled};data.setHandshakeTimeoutMs(5000);data.setIoTimeoutMs(2000);
 ftp.control.print("RETR "+path+"\r\n");
 if(!data.connect(job.host,p1*256+p2,4000,&ftp.control,job.plainData))return 6;
 int reply=ftp.reply();if(reply!=150&&reply!=125)return 6;
 size_t consumed=0;
 auto raw=[&](uint8_t *dest,size_t bytes)->int{
  if(consumed>=fileSize)return 0;
  bytes=std::min(bytes,size_t(fileSize-consumed));uint32_t progress=millis();
  while(!cancelled()){
   int available=data.available();
   if(available>0)return data.read(dest,std::min(bytes,size_t(available)));
   if(!data.connected()||uint32_t(millis()-progress)>3000)return 0;
   delay(1);
  }return 0;
 };
 int result=XTouchPreviewArchive::extract(raw,cancelled,allocate,job.plate,png,pngSize,consumed);
 data.stop();ftp.control.print("ABOR\r\n");
 Serial.printf("[PREVIEW] archive prefix=%u limit=%u result=%d\n",unsigned(consumed),unsigned(XTouchPreviewArchive::scanLimit),result);

 return result;
}

int fetch(uint8_t *&png,size_t &pngSize){
 String file=job.file;return file.endsWith(".png")?fetchPng(png,pngSize):fetchArchive(png,pngSize);
}
// Cloud history supplies a separate PNG cover. Match the exact task, printer and
// plate; never take the latest entry blindly and never follow archive URLs.
int fetchCloud(uint8_t *&png,size_t &pngSize){
 String cover;bool verifiedTask=false;networkStage=1;networkStatus=0;tlsStatus=0;
 if(!XTouchPreview::validTaskId(job.task))return 25;
 for(unsigned route=0;route<3&&cover.isEmpty()&&!cancelled();++route){
  if(route==1&&!verifiedTask)continue;
  WiFiClientSecure tls;tls.setCACert(xtouch_preview_api_ca);tls.setHandshakeTimeout(6);tls.setTimeout(2);
  HTTPClient http;http.setConnectTimeout(6000);http.setTimeout(2000);http.useHTTP10(false);http.setReuse(false);
  String base=XTouchCloudEndpoints::get(job.china?"cn":"global")->api;
  String endpoint=route==0?base+"/v1/user-service/my/task/"+job.task:
   route==1?base+"/v1/iot-service/api/user/task/"+job.task:
   String(XTouchPreview::taskApi(job.china))+"&deviceId="+job.serial;
  const char *headers[]={"Transfer-Encoding"};http.collectHeaders(headers,1);
  if(!http.begin(tls,endpoint))return 26;
  http.addHeader("Accept","application/json");
  http.addHeader("User-Agent","xTouch-CN/1.0 (+https://github.com/tbc0309/xTouch-CN)");
  http.addHeader("Authorization",String("Bearer ")+job.token);
  networkStatus=http.GET();if(networkStatus!=200){char detail[128];tlsStatus=tls.lastError(detail,sizeof(detail));http.end();if(networkStatus==404&&route<2)continue;return 26;}
  class Body : public Stream {
   WiFiClient *source;size_t budget=262144,chunk=0;bool chunked,afterChunk=false,done=false;int cached=-1;uint8_t buffer[512];size_t cursor=0,buffered=0;
   int raw(){
    if(cancelled()||!budget)return -1;
    if(cursor<buffered){--budget;return buffer[cursor++];}
    uint32_t at=millis();
    while(!cancelled()&&uint32_t(millis()-at)<2000){
     int available=source->available();
     if(available){int n=source->read(buffer,std::min(sizeof(buffer),size_t(available)));if(n>0){cursor=1;buffered=n;--budget;return buffer[0];}}
     else if(!source->connected())return -1;
     delay(1);
    }
    return -1;
   }
   int next(){
    if(done)return -1;
    if(!chunked)return raw();
    if(!chunk){
     if(afterChunk&&(raw()!='\r'||raw()!='\n')){done=true;return -1;}
     char line[128];unsigned n=0;int c;
     while((c=raw())>=0&&c!='\r'&&n<sizeof(line)-1)line[n++]=char(c);
     if(c!='\r'||raw()!='\n'||!n){done=true;return -1;}
     line[n]=0;char *end=nullptr;unsigned long length=strtoul(line,&end,16);
     if(end==line||(*end&&*end!=';')||length>budget||!length){done=true;return -1;}
     chunk=length;afterChunk=true;
    }
    int c=raw();if(c<0){done=true;return -1;}--chunk;return c;
   }
  public:
   Body(WiFiClient *s,bool encoded):source(s),chunked(encoded){setTimeout(2000);}
   int available()override{return cached>=0?1:done?0:cursor<buffered?buffered-cursor:source->available();}
   int read()override{if(cached>=0){int c=cached;cached=-1;return c;}return next();}
   int peek()override{if(cached<0)cached=next();return cached;}
   size_t write(uint8_t)override{return 0;}
   void flush()override{}
  } body(http.getStreamPtr(),http.header("Transfer-Encoding").equalsIgnoreCase("chunked"));

  StaticJsonDocument<256> filter;
  if(route==1){filter["index"]=true;filter["thumbnail"]["url"]=true;}
  else for(const char *key:{"id","deviceId","plateIndex","cover"})filter[key]=true;
  StaticJsonDocument<2048> response;
  auto nonspace=[&](){int c;do{c=body.read();}while(c==' '||c=='\r'||c=='\n'||c=='\t');return c;};
  if(route==0){
   auto error=deserializeJson(response,body,DeserializationOption::Filter(filter));
   if(!error){String id=response["id"].as<String>();
    verifiedTask=XTouchPreview::matchesTask(id.c_str(),response["deviceId"]|"",response["plateIndex"].as<unsigned>(),job.task,job.serial,job.plate);
    if(verifiedTask)cover=response["cover"]|"";
   }
   http.end();continue;
  }
  bool found=body.find(route==1?"\"plates\"":"\"hits\"");
  if(!found||nonspace()!=':'||nonspace()!='['){http.end();if(route==1)continue;return 27;}
  for(unsigned count=0;count<100&&!cancelled();++count){
   while(body.peek()==' '||body.peek()=='\r'||body.peek()=='\n'||body.peek()=='\t')body.read();
   if(body.peek()==']')break;
   auto error=deserializeJson(response,body,DeserializationOption::Filter(filter));
   if(error){networkStatus=-int(error.code());http.end();return 27;}
   String id=response["id"].as<String>();
   if(route==1){
    if(response["index"].as<unsigned>()==job.plate){cover=response["thumbnail"]["url"]|"";break;}
   }else if(XTouchPreview::matchesTask(id.c_str(),response["deviceId"]|"",response["plateIndex"].as<unsigned>(),job.task,job.serial,job.plate)){cover=response["cover"]|"";break;}
   int separator=nonspace();if(separator==']')break;if(separator!=','){http.end();return 27;}
   response.clear();
  }
  http.end();
 }
 if(cancelled()||cover.isEmpty()||cover.length()>2048)return 25;
 int slash=cover.indexOf('/',8);String host=slash>8?cover.substring(8,slash):String();
 if(!cover.startsWith("https://")||cover.indexOf('\r')>=0||cover.indexOf('\n')>=0||host.indexOf('@')>=0||host.indexOf(':')>=0)return 28;
 // Public Bambu CDN only. Authorization is never sent to the image host.
 if(!XTouchPreview::trustedCoverHost(host.c_str()))return 28;
 String path=cover.substring(slash);int query=path.indexOf('?');if(query>=0)path=path.substring(0,query);path.toLowerCase();if(!path.endsWith(".png"))return 28;
 networkStage=2;
WiFiClientSecure tls;tls.setCACert((host=="bbl-prod-model.oss-cn-shanghai.aliyuncs.com"||host=="bbl-us-public.oss-us-west-1.aliyuncs.com")?xtouch_preview_ca:xtouch_cloud_trust());
 tls.setHandshakeTimeout(6);tls.setTimeout(2);
 HTTPClient http;http.setConnectTimeout(6000);http.setTimeout(2000);http.useHTTP10(false);http.setReuse(false);
 if(!http.begin(tls,cover))return 26;
 networkStatus=http.GET();if(networkStatus!=200){char detail[128];tlsStatus=tls.lastError(detail,sizeof(detail));tlsPhase=0;http.end();return 26;}
 int length=http.getSize();
 const size_t cap=262144;
 if(length<=0||unsigned(length)>cap){http.end();return 11;}
 png=(uint8_t*)allocate(length);if(!png){http.end();return 12;}
 WiFiClient *stream=http.getStreamPtr();uint32_t progress=millis();
 while(pngSize<unsigned(length)&&!cancelled()){
  int available=stream->available();
  if(available>0){uint8_t scratch[512];int n=stream->read(scratch,std::min(sizeof(scratch),std::min(size_t(available),size_t(length)-pngSize)));if(n>0)memcpy(png+pngSize,scratch,n);if(n<=0)break;pngSize+=n;progress=millis();}
  else {if(!http.connected()||uint32_t(millis()-progress)>2000)break;delay(1);}
 }
 http.end();
 if(cancelled()||pngSize!=unsigned(length)){free(png);png=nullptr;return 13;}
 return 0;
}
struct Decode {PNG *png;uint16_t *pixels;unsigned sourceW,sourceH,w,h,next=0;uint16_t row[512];};
int draw(PNGDRAW *line){
 Decode *d=(Decode*)line->pUser;if(cancelled())return 0;
 d->png->getLineAsRGB565(line,d->row,PNG_RGB565_LITTLE_ENDIAN,0x555555);
 while(d->next<d->h&&d->next*d->sourceH/d->h==unsigned(line->y)){
  for(unsigned x=0;x<d->w;++x){uint16_t color=d->row[x*d->sourceW/d->w];size_t index=d->next*d->w+x;d->pixels[index]=color;}++d->next;
 }delay(1);return 1;
}
int decode(uint8_t *bytes,size_t size){
 std::unique_ptr<PNG> png(new(std::nothrow) PNG);if(!png)return 18;
 int opened=png->openRAM(bytes,size,draw);
 if(opened!=PNG_SUCCESS)return 19;
 unsigned w=png->getWidth(),h=png->getHeight();
 if(!w||!h||w>512||h>512||png->isInterlaced()){png->close();return 20;}
#ifdef XTOUCH_BOARD_P4
 unsigned limit=512;
#else
 unsigned limit=128;
#endif
 unsigned dw=w,dh=h;if(dw>limit||dh>limit){dw=w>=h?limit:w*limit/h;dh=h>=w?limit:h*limit/w;}
 if(!dw||!dh){png->close();return 20;}
 uint16_t *pixels=(uint16_t*)allocate(dw*dh*2);if(!pixels){png->close();return 21;}
 Decode context={};context.png=png.get();context.pixels=pixels;context.sourceW=w;context.sourceH=h;context.w=dw;context.h=dh;int rc=png->decode(&context,PNG_CHECK_CRC);png->close();
 if(rc!=PNG_SUCCESS||context.next!=dh||cancelled()){free(pixels);return 22;}
 ready={pixels,dw,dh};return 0;
}
void worker(void*){
 Serial.println("[PREVIEW] worker=zip-compatible-v5");
 networkStage=0;networkStatus=0;tlsStatus=0;tlsPhase=0;
 started=millis();uint8_t *png=nullptr;size_t size=0;lastResult=job.token[0]?fetchCloud(png,size):fetch(png,size);
 if(!lastResult&&!cancelled()){networkStage=3;lastResult=decode(png,size);}
 free(png);
 memset(job.code,0,sizeof(job.code));memset(job.token,0,sizeof(job.token));
 Serial.printf("[PREVIEW] result=%d bytes=%u image=%ux%u elapsed=%u heap=%u\n",lastResult,unsigned(size),ready.w,ready.h,unsigned(millis()-started),unsigned(ESP.getFreeHeap()));
 complete.store(true,std::memory_order_release);running=false;vTaskDelete(nullptr);
}
void clearShown(){renderedLines=0;ui_home_preview_set(nullptr);lv_img_cache_invalidate_src(&descriptor);free(shown.pixels);shown={};descriptor={};}
}

XTouchPrintPreviewStatus xtouch_print_preview_status(){return {running.load(),shown.pixels!=nullptr,shown.w,shown.h,tries,publishedResult,networkStage.load(),networkStatus.load(),tlsStatus.load(),renderedLines.load(),tlsPhase.load()};}
void xtouch_print_preview_suspend(bool value){
 if(suspended.exchange(value)==value)return;
 ++generation;clearShown();activeKey="";tries=0;nextAttempt=millis()+1200;
}
void xtouch_print_preview_poll(){
 if(suspended){
  if(complete.exchange(false,std::memory_order_acquire)){free(ready.pixels);ready={};}
  return;
 }
 bool printing=bambuStatus.print_status==XTOUCH_PRINT_STATUS_RUNNING||bambuStatus.print_status==XTOUCH_PRINT_STATUS_PAUSED||bambuStatus.print_status==XTOUCH_PRINT_STATUS_PREPARE;
 const auto source=XTouchPreview::source(xtouch_printer_cloud(),xtouch_cloud_simulator());
 const char *task=source==XTouchPreview::Source::Cloud?XTouchPreview::cloudTaskId(bambuStatus.obj_subtask_id,bambuStatus.task_id):bambuStatus.task_id;
 String key=printing?String(int(source))+"|"+xtouch_printer_account()+"|"+(source==XTouchPreview::Source::Local?"":xtouch_cloud_broker())+"|"+(source==XTouchPreview::Source::Cloud?xtouch_cloud_user():"")+"|"+xTouchConfig.xTouchSerialNumber+"|"+bambuStatus.gcode_file+"|"+bambuStatus.subtask_name+"|"+task+"|"+bambuStatus.plate_index:String();
 if(key!=activeKey){generation++;activeKey=key;tries=0;nextAttempt=millis()+1200;clearShown();}
 if(complete.exchange(false,std::memory_order_acquire)){
  publishedResult=lastResult;
  if(job.generation==generation.load()&&ready.pixels&&!lastResult){
   clearShown();shown=ready;ready={};descriptor.header.cf=LV_IMG_CF_TRUE_COLOR;descriptor.header.w=shown.w;descriptor.header.h=shown.h;
   descriptor.data=(const uint8_t*)shown.pixels;descriptor.data_size=shown.w*shown.h*2;
  }else{free(ready.pixels);ready={};}
  unsigned wait=XTouchPreview::retryDelay(publishedResult,networkStatus.load(),tries);
  nextAttempt=millis()+(job.generation==generation.load()?(wait?wait:30000):1200);
 }
 if(shown.pixels){if(xTouchConfig.currentScreenIndex==0)ui_home_preview_set(&descriptor);return;}
 bool retry=XTouchPreview::retryDelay(publishedResult,networkStatus.load(),tries)!=0;
 if(!printing||running||complete||activeKey.isEmpty()||(tries>=1&&!retry)||millis()<30000||int32_t(millis()-nextAttempt)<0||WiFi.status()!=WL_CONNECTED||xtouch_cloud_busy()||xtouch_portal_maintenance()||ui_scroll_busy(750))return;
 job.port=990;job.token[0]=0;
 snprintf(job.serial,sizeof(job.serial),"%s",xTouchConfig.xTouchSerialNumber);snprintf(job.task,sizeof(job.task),"%s",task);
 if(source==XTouchPreview::Source::Cloud){
  if(!strcmp(job.task,"0")||!job.task[0]||!xtouch_cloud_preview_credentials(job.token,sizeof(job.token),job.china))return;
 }
 const char *model=xtouch_printer_model_at(xtouch_printer_active());job.plainData=model&&(!strcmp(model,"N1")||!strcmp(model,"N2S")||!strcmp(model,"A1")||!strcmp(model,"A1 Mini"));
 if(source==XTouchPreview::Source::Simulator){
  snprintf(job.host,sizeof(job.host),"%s",xtouch_cloud_broker());snprintf(job.code,sizeof(job.code),"12345678");job.port=1990;job.plainData=false;
 }else if(source==XTouchPreview::Source::Local&&!xtouch_printer_local_credentials(job.host,sizeof(job.host),job.code,sizeof(job.code)))return;
 if(source!=XTouchPreview::Source::Cloud&&!bambuStatus.gcode_file[0]&&!bambuStatus.subtask_name[0])return;
 snprintf(job.file,sizeof(job.file),"%s",bambuStatus.gcode_file);snprintf(job.title,sizeof(job.title),"%s",bambuStatus.subtask_name);job.plate=bambuStatus.plate_index>0?bambuStatus.plate_index:1;job.generation=generation.load();
 // A thumbnail must never suspend the printer's MQTT connection.
 // Unsupported sources retain the model image; retries remain bounded.
 ++tries;running=true;
 if(xTaskCreate(worker,"print-preview",8192,nullptr,1,nullptr)!=pdPASS){running=false;nextAttempt=millis()+30000;}
}

#endif
