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
#include "platform/idf/runtime.h"
#include "platform/idf/wifi.h"
#include "preview_transport.h"
#include "preview_ftp_cleanup.h"
#include "features/network/ftps_session.h"
#include "features/diagnostics/stack_margin.h"
#include "features/network/task_budget.h"
#include <PNGdec.h>
#include "png_pixels.h"
#include "xtouch/http_client.h"
#include "preview_ca.h"
#include "preview_api_ca.h"

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
int publishedResult=0;std::atomic<unsigned> networkStage{0};std::atomic<int> networkStatus{0},tlsStatus{0},jsonStatus{0};
bool cancelled(){return suspended.load()||generation.load()!=job.generation||xtouch_portal_maintenance()||uint32_t(millis()-started)>20000;}
void *allocate(size_t n){
 void *p=heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 return p?p:heap_caps_malloc(n,MALLOC_CAP_8BIT);
}
std::atomic<int> ftpTransferReply{0},ftpAbortReply{0},ftpQuitReply{0};
std::atomic<unsigned> ftpCleanupResult{0};
void ftpClosed(const XTouchPreviewFtp::CloseResult &r){ftpTransferReply=r.transfer;ftpAbortReply=r.abort;ftpQuitReply=r.quit;ftpCleanupResult=r.confirmed?1:r.released?3:2;}
struct Ftps:xtouch_network::FtpsSession {
 Ftps():FtpsSession(job.host,job.code,job.port,job.plainData,cancelled,ftpClosed){}
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
 auto &data=ftp.data;data.setHandshakeTimeoutMs(5000);data.setIoTimeoutMs(2000);
 if(!ftp.requestTransfer(file))return fail(6);
 if(!data.connect(job.host,p1*256+p2,4000,&ftp.control,job.plainData))return fail(6);
 int reply=ftp.reply();if(reply!=150&&reply!=125)return fail(6);
 uint32_t progress=millis();
 while(pngSize<fileSize&&!cancelled()){
  int available=data.available();
  if(available>0){uint8_t scratch[512];int n=data.read(scratch,std::min(sizeof(scratch),std::min(size_t(available),size_t(fileSize-pngSize))));if(n>0)memcpy(png+pngSize,scratch,n);if(n<=0)return fail(13);pngSize+=n;ftp.received+=size_t(n);progress=millis();}
  else {if(!data.connected()||uint32_t(millis()-progress)>3000)return fail(13);delay(1);}
 }
 if(cancelled()||pngSize!=fileSize)return fail(13);
 ftp.transfer=XTouchPreviewFtp::Transfer::Complete;
 if(!ftp.finish())return fail(13);
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
 auto &data=ftp.data;data.setHandshakeTimeoutMs(5000);data.setIoTimeoutMs(2000);
 if(!ftp.requestTransfer(path))return 6;
 if(!data.connect(job.host,p1*256+p2,4000,&ftp.control,job.plainData))return 6;
 int reply=ftp.reply();if(reply!=150&&reply!=125)return 6;
 size_t consumed=0;
 auto raw=[&](uint8_t *dest,size_t bytes)->int{
  if(consumed>=fileSize)return 0;
  bytes=std::min(bytes,size_t(fileSize-consumed));uint32_t progress=millis();
  while(!cancelled()){
   int available=data.available();
   if(available>0){int n=data.read(dest,std::min(bytes,size_t(available)));if(n>0)ftp.received+=size_t(n);return n;}
   if(!data.connected()||uint32_t(millis()-progress)>3000)return 0;
   delay(1);
  }return 0;
 };
 int result=XTouchPreviewArchive::extract(raw,cancelled,allocate,job.plate,png,pngSize,consumed);
 size_t prefixBytes=consumed;
 // Stop as soon as the image is extracted. Never drain the rest of the archive.
 ftp.transfer=consumed>=fileSize?XTouchPreviewFtp::Transfer::Complete:XTouchPreviewFtp::Transfer::Prefix;
 bool closedCleanly=ftp.finish();
 if(!closedCleanly&&!result){free(png);png=nullptr;pngSize=0;result=13;}
 Serial.printf("[PREVIEW] archive prefix=%u read=%u file=%u limit=%u result=%d\n",unsigned(prefixBytes),unsigned(ftp.received),unsigned(fileSize),unsigned(XTouchPreviewArchive::scanLimit),result);

 return result;
}

int fetch(uint8_t *&png,size_t &pngSize){
 String file=job.file;return file.endsWith(".png")?fetchPng(png,pngSize):fetchArchive(png,pngSize);
}
// Cloud history supplies a separate PNG cover. Match the exact task, printer and
// plate; never take the latest entry blindly and never follow archive URLs.
int fetchCloud(uint8_t *&png,size_t &pngSize){
 String cover;bool verifiedTask=false;networkStage=1;networkStatus=0;tlsStatus=0;jsonStatus=0;
 if(!XTouchPreview::validTaskId(job.task))return 25;
 for(unsigned route=0;route<3&&cover.isEmpty()&&!cancelled();++route){
  if(route==1&&!verifiedTask)continue;
  XTouchHttpClient http;http.setConnectTimeout(6000);http.setTimeout(2000);
  String base=XTouchCloudEndpoints::get(job.china?"cn":"global")->api;
  String endpoint=route==0?base+"/v1/user-service/my/task/"+job.task:
   route==1?base+"/v1/iot-service/api/user/task/"+job.task:
   String(XTouchPreview::taskApi(job.china))+"&deviceId="+job.serial;
  if(!http.begin(endpoint,xtouch_preview_api_ca))return 26;
  http.addHeader("Accept","application/json");
  http.addHeader("User-Agent","xTouch-CN/1.0 (+https://github.com/tbc0309/xTouch-CN)");
  http.addHeader("Authorization",String("Bearer ")+job.token);
  networkStatus=http.GET();XTouchStackMargin::sample(XTouchStackMargin::Path::CloudPreview,true);if(networkStatus!=200){tlsStatus=networkStatus.load()<0?networkStatus.load():0;http.end();if(networkStatus==404&&route<2)continue;return 26;}
  // IDF supplies decoded body bytes; keep only the application cap/cancellation.
  class Body : public Stream {
   Stream *source;size_t remaining=262144;
  public:
   explicit Body(Stream *s):source(s){setTimeout(2000);}
   int available()override{return cancelled()||!remaining?0:std::min(size_t(source->available()),remaining);}
   int read()override{if(cancelled()||!remaining)return -1;int c=source->read();if(c>=0)--remaining;return c;}
   int peek()override{return cancelled()||!remaining?-1:source->peek();}
   size_t write(uint8_t)override{return 0;}
   void flush()override{}
  } body(http.getStreamPtr());

  StaticJsonDocument<256> filter;
  if(route==1){filter["index"]=true;filter["thumbnail"]["url"]=true;}
  else for(const char *key:{"id","deviceId","plateIndex","cover"})filter[key]=true;
  DynamicJsonDocument response(2048);
  if(response.capacity()<2048){http.end();return 12;}
  auto nonspace=[&](){int c;do{c=body.read();}while(c==' '||c=='\r'||c=='\n'||c=='\t');return c;};
  if(route==0){
   auto error=deserializeJson(response,body,DeserializationOption::Filter(filter));
   jsonStatus=int(error.code());
   if(!error){String id=response["id"].as<String>();
    verifiedTask=XTouchPreview::matchesCloudTask(id.c_str(),response["deviceId"]|"",response["plateIndex"].as<unsigned>(),job.task,job.serial,job.plate);
    if(verifiedTask){job.plate=response["plateIndex"].as<unsigned>();cover=response["cover"]|"";}
   }
   http.end();continue;
  }
  bool found=body.find(route==1?"\"plates\"":"\"hits\"");
  if(!found||nonspace()!=':'||nonspace()!='['){http.end();if(route==1)continue;return 27;}
  for(unsigned count=0;count<100&&!cancelled();++count){
   while(body.peek()==' '||body.peek()=='\r'||body.peek()=='\n'||body.peek()=='\t')body.read();
   if(body.peek()==']')break;
   auto error=deserializeJson(response,body,DeserializationOption::Filter(filter));
   if(error){jsonStatus=int(error.code());http.end();return 27;}
   jsonStatus=0;
   String id=response["id"].as<String>();
   if(route==1){
    if(response["index"].as<unsigned>()==job.plate){cover=response["thumbnail"]["url"]|"";break;}
   }else if(XTouchPreview::matchesCloudTask(id.c_str(),response["deviceId"]|"",response["plateIndex"].as<unsigned>(),job.task,job.serial,job.plate)){job.plate=response["plateIndex"].as<unsigned>();cover=response["cover"]|"";break;}
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
 XTouchHttpClient http;http.setConnectTimeout(6000);http.setTimeout(2000);
 const char *trust=(host=="bbl-prod-model.oss-cn-shanghai.aliyuncs.com"||host=="bbl-us-public.oss-us-west-1.aliyuncs.com")?xtouch_preview_ca:xtouch_cloud_trust();
 if(!http.begin(cover,trust))return 26;
 networkStatus=http.GET();XTouchStackMargin::sample(XTouchStackMargin::Path::CloudPreview,true);if(networkStatus!=200){tlsStatus=networkStatus.load()<0?networkStatus.load():0;tlsPhase=0;http.end();return 26;}
 int length=http.getSize();
 const size_t cap=262144;
 if(length==0||(length>0&&unsigned(length)>cap)){http.end();return 11;}
 const size_t limit=length>0?size_t(length):cap;
 png=(uint8_t*)allocate(limit);if(!png){http.end();return 12;}
 Stream *stream=http.getStreamPtr();uint32_t progress=millis();
 while(pngSize<limit&&!cancelled()){
  int available=stream->available();
  if(available>0){uint8_t scratch[512];int n=stream->readBytes(scratch,std::min(sizeof(scratch),std::min(size_t(available),limit-pngSize)));if(n>0)memcpy(png+pngSize,scratch,n);if(n<=0)break;pngSize+=n;progress=millis();}
  else {if(!http.connected()||uint32_t(millis()-progress)>2000)break;delay(1);}
 }
 bool oversized=length<0&&pngSize==cap&&stream->available()>0;
 http.end();
 if(cancelled()||oversized||!pngSize||(length>0&&pngSize!=unsigned(length))){free(png);png=nullptr;return oversized?11:13;}
 return 0;
}
struct Decode {PNG *png;uint16_t *pixels;unsigned sourceW,sourceH,w,h,next=0;};
int draw(PNGDRAW *line){
 Decode *d=(Decode*)line->pUser;if(cancelled())return 0;
 while(d->next<d->h&&d->next*d->sourceH/d->h==unsigned(line->y)){
  size_t offset=size_t(d->next)*d->w;
  if(!XTouchPngPixels::row(*d->png,*line,d->pixels+offset,XTouchPngPixels::alpha(d->pixels,d->w,d->h)+offset,d->w,d->sourceW))return 0;
  ++d->next;
 }delay(1);return 1;
}
int decode(uint8_t *bytes,size_t size){
 std::unique_ptr<PNG> png(new(std::nothrow) PNG);if(!png)return 18;
 int opened=png->openRAM(bytes,size,draw);
 if(opened!=PNG_SUCCESS)return 19;
 unsigned w=png->getWidth(),h=png->getHeight();
 if(!w||!h||w>512||h>512||png->isInterlaced()){png->close();return 20;}
 constexpr unsigned limit=XTouchPreviewArchive::thumbnailEdge;
 unsigned dw=w,dh=h;if(dw>limit||dh>limit){dw=w>=h?limit:w*limit/h;dh=h>=w?limit:h*limit/w;}
 if(!dw||!dh){png->close();return 20;}
 uint16_t *pixels=(uint16_t*)allocate(XTouchPngPixels::bytes(dw,dh));if(!pixels){png->close();return 21;}
 Decode context={};context.png=png.get();context.pixels=pixels;context.sourceW=w;context.sourceH=h;context.w=dw;context.h=dh;int rc=png->decode(&context,PNG_CHECK_CRC);png->close();
 if(rc!=PNG_SUCCESS||context.next!=dh||cancelled()){free(pixels);return 22;}
 ready={pixels,dw,dh};return 0;
}
constexpr unsigned workerStackBytes=XTouchNetworkBudgets::PreviewWorkerStackBytes;
std::atomic<unsigned> workerStackMin{0};
void worker(void*){
 Serial.println("[PREVIEW] worker=zip-compatible-v5");
 networkStage=0;networkStatus=0;tlsStatus=0;tlsPhase=0;jsonStatus=0;
 ftpTransferReply=0;ftpAbortReply=0;ftpQuitReply=0;ftpCleanupResult=0;
 auto stackPath=job.token[0]?XTouchStackMargin::Path::CloudPreview:XTouchStackMargin::Path::LocalPreview;
 XTouchStackMargin::sample(stackPath,true);
 started=millis();uint8_t *png=nullptr;size_t size=0;lastResult=job.token[0]?fetchCloud(png,size):fetch(png,size);
 if(!lastResult&&!cancelled()){networkStage=3;lastResult=decode(png,size);}
 XTouchStackMargin::sample(stackPath,true);
 workerStackMin=uxTaskGetStackHighWaterMark(nullptr);
 Serial.printf("[PREVIEW] stack-min=%u budget=%u bytes\n",workerStackMin.load(),workerStackBytes);
 free(png);
 memset(job.code,0,sizeof(job.code));memset(job.token,0,sizeof(job.token));
 Serial.printf("[PREVIEW] result=%d bytes=%u image=%ux%u elapsed=%u heap=%u\n",lastResult,unsigned(size),ready.w,ready.h,unsigned(millis()-started),unsigned(ESP.getFreeHeap()));
 complete.store(true,std::memory_order_release);running=false;vTaskDelete(nullptr);
}
void clearShown(){renderedLines=0;ui_home_preview_set(nullptr);lv_img_cache_invalidate_src(&descriptor);free(shown.pixels);shown={};descriptor={};}
}

XTouchPrintPreviewStatus xtouch_print_preview_status(){return {running.load(),shown.pixels!=nullptr,shown.w,shown.h,tries,publishedResult,networkStage.load(),networkStatus.load(),tlsStatus.load(),renderedLines.load(),tlsPhase.load(),jsonStatus.load(),workerStackBytes,workerStackMin.load(),ftpTransferReply.load(),ftpAbortReply.load(),ftpQuitReply.load(),ftpCleanupResult.load()};}
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
 String key=printing?String(int(source))+"|"+(source==XTouchPreview::Source::Local?xTouchConfig.xTouchHost:"")+"|"+xtouch_printer_account()+"|"+(source==XTouchPreview::Source::Local?"":xtouch_cloud_broker())+"|"+(source==XTouchPreview::Source::Cloud?xtouch_cloud_user():"")+"|"+xTouchConfig.xTouchSerialNumber+"|"+bambuStatus.gcode_file+"|"+bambuStatus.subtask_name+"|"+task+"|"+bambuStatus.plate_index+"|"+(bambuStatus.plate_index_valid?1:0):String();
 if(key!=activeKey){generation++;activeKey=key;tries=0;nextAttempt=millis()+1200;clearShown();}
 if(complete.exchange(false,std::memory_order_acquire)){
  publishedResult=lastResult;
  if(job.generation==generation.load()&&ready.pixels&&!lastResult){
   clearShown();shown=ready;ready={};descriptor.header.magic=LV_IMAGE_HEADER_MAGIC;descriptor.header.cf=LV_COLOR_FORMAT_RGB565A8;descriptor.header.w=shown.w;descriptor.header.stride=shown.w*2;descriptor.header.h=shown.h;
   descriptor.data=(const uint8_t*)shown.pixels;descriptor.data_size=XTouchPngPixels::bytes(shown.w,shown.h);
  }else{free(ready.pixels);ready={};}
  unsigned wait=XTouchPreview::retryDelay(publishedResult,networkStatus.load(),tries,jsonStatus.load());
  nextAttempt=millis()+(job.generation==generation.load()?(wait?wait:30000):1200);
 }
 if(shown.pixels){if(xTouchConfig.currentScreenIndex==0)ui_home_preview_set(&descriptor);return;}
 bool retry=XTouchPreview::retryDelay(publishedResult,networkStatus.load(),tries,jsonStatus.load())!=0;
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
 snprintf(job.file,sizeof(job.file),"%s",bambuStatus.gcode_file);snprintf(job.title,sizeof(job.title),"%s",bambuStatus.subtask_name);job.plate=source==XTouchPreview::Source::Cloud&&!bambuStatus.plate_index_valid?0:bambuStatus.plate_index>0?bambuStatus.plate_index:1;job.generation=generation.load();
 // A thumbnail must never suspend the printer's MQTT connection.
 // Unsupported sources retain the model image; retries remain bounded.
 ++tries;running=true;
 if(xTaskCreate(worker,"print-preview",workerStackBytes,nullptr,1,nullptr)!=pdPASS){running=false;nextAttempt=millis()+30000;}
}

#endif
