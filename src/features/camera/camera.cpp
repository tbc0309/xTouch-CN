#include "features/config.h"
#if XTOUCH_FEATURE_CAMERA
#include "camera.h"
#include "camera_ui.h"
#include "camera_protocol.h"
#include "camera_frame.h"
#include "camera_messages.h"
#include "camera_rtsp.h"
#include "frame_exchange.h"
#include "features/network/task_budget.h"
#include "features/diagnostics/stack_margin.h"
#include <esp_heap_caps.h>
#include "platform/idf/runtime.h"
#include "platform/idf/wifi.h"
#include "platform/idf/socket.h"
#include <JPEGDEC.h>
#include "xtouch/types.h"
#include "xtouch/printers.h"
#include "xtouch/portal.h"
#include "ui/i18n/language.h"
#include "ui/ui_msgs.h"
#include "ui/printer_models.h"
#include <freertos/semphr.h>
#include <atomic>
#include <new>
#include <utility>
#include <esp_pm.h>
#include "features/preview/print_preview.h"
extern "C" lv_timer_t *xtouch_screen_onScreenOffTimer;
namespace {
int cameraWidth,cameraHeight;
std::atomic<unsigned> connections{0},lastFrame{0};
std::atomic<bool> running{false},stop{false};std::atomic<unsigned> frames{0};std::atomic<int> result{0};
SemaphoreHandle_t lock;uint16_t *decoded,*shown,*ready;uint8_t *jpegBytes;JPEGDEC *decoder;
char host[64],code[9],serial[32],account[32];bool cloudSource=false;CameraUi view;lv_img_dsc_t descriptor;unsigned displayed;int lastResult=-1;
bool paused=false,wantRestart=false;uint32_t lastLight,decodeStarted,lastYield;
bool useRtsps=false;
bool wifiPowerHeld=false;wifi_ps_type_t previousWifiPower=WIFI_PS_MIN_MODEM;
std::atomic<unsigned> frameWidth{0},frameHeight{0};unsigned lastWidth=0,lastHeight=0,lastLanguage=255;
const char *msg(int n){return cameraMessage(n,xtouch_language_get());}
enum class CloseReason : unsigned { None, User, Suspend, Network, Maintenance, Source, Credentials };
CloseReason lastClose=CloseReason::None;
void closeView(CloseReason reason){
 stop=true;wantRestart=false;
 if(view.overlay){
  lastClose=reason;Serial.printf("[CAMERA] closed reason=%u state=%d frames=%u\n",unsigned(reason),int(result),unsigned(frames));
  lv_obj_del(view.overlay);lv_img_cache_invalidate_src(&descriptor);view={};
 }
}
void action(CameraAction a){
 if(a==CameraAction::Close){closeView(CloseReason::User);return;}
 if(strcmp(serial,xTouchConfig.xTouchSerialNumber)){closeView(CloseReason::Source);return;}
 if(a==CameraAction::Light){if(xtouch_printer_online()&&uint32_t(millis()-lastLight)>=1000){lastLight=millis();lv_msg_send(XTOUCH_COMMAND_LIGHT_TOGGLE,nullptr);}return;}
 if(a==CameraAction::Freeze){if(result==6)paused=!paused;return;}
 if(a==CameraAction::Retry){stop=true;wantRestart=true;paused=false;result=8;}
}
bool cancelled(){return stop||uint32_t(millis()-decodeStarted)>=CameraFrame::decodeBudgetMs;}
int draw(JPEGDRAW *p){
 if(cancelled())return 0;
 for(int yy=0;yy<p->iHeight;++yy){int y=p->y+yy;if(y<0||y>=cameraHeight)continue;for(int xx=0;xx<p->iWidth;++xx){int x=p->x+xx;if(x>=0&&x<cameraWidth)decoded[y*cameraWidth+x]=p->pPixels[yy*p->iWidth+xx];}}
 if(uint32_t(millis()-lastYield)>=8){lastYield=millis();vTaskDelay(1);}
 return !cancelled();
}
bool readExact(WiFiClientSecure &client,uint8_t *p,size_t n,bool header=false){return CameraProtocol::receive(client,p,n,[]{return uint32_t(millis());},[]{vTaskDelay(pdMS_TO_TICKS(5));},[]{return bool(stop);},header?20000:15000,header?20000:5000);}
bool waitRetry(unsigned failures){uint32_t start=millis(),delay=failures==1?500:failures==2?1000:2000;while(!stop&&uint32_t(millis()-start)<delay)vTaskDelay(pdMS_TO_TICKS(20));return !stop;}
void publishFrame(unsigned w,unsigned h){
 if(xSemaphoreTake(lock,pdMS_TO_TICKS(10))!=pdTRUE)return;
 CameraBuffer::publish(decoded,ready);
 frameWidth=w;frameHeight=h;++frames;lastFrame=millis();xSemaphoreGive(lock);
}
void worker(void *){
 auto stackPath=useRtsps?XTouchStackMargin::Path::H264:XTouchStackMargin::Path::Jpeg;
 XTouchStackMargin::sample(stackPath,true);
 Serial.printf("[CAMERA] worker core=%d\n", int(xPortGetCoreID()));
 // Keep decoding on CPU0; the UI task affinity is selected in sdkconfig.
 esp_pm_lock_handle_t frequencyLock=nullptr;
 if(esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX,0,"camera-playback",&frequencyLock)==ESP_OK)esp_pm_lock_acquire(frequencyLock);
 unsigned failures=0;
 while(!stop){
  ++connections;
  if(useRtsps){
   CameraRtspSink sink{[]{return decoded;},unsigned(cameraWidth),unsigned(cameraHeight),publishFrame,[](int state){result=state;}};
   unsigned before=frames;int failure=cameraRtspRun(host,code,stop,sink);if(stop)break;if(frames!=before)failures=0;result=failure;
   if(failure==3||failure==5||failure==7||failure==11||failure==12||++failures>=3)break;
   result=8;if(!waitRetry(failures))break;continue;
  }
  WiFiClientSecure c;c.setInsecure(); // Printer self-signed LAN certificates; private IPv4 enforced before starting.
  c.setTimeout(1);c.setHandshakeTimeout(3);uint8_t auth[80];int failure=1;
  if(CameraProtocol::auth(code,auth)&&c.connect(host,6000,3000)&&c.write(auth,sizeof(auth))==sizeof(auth)){
   while(!stop){
    uint8_t header[16];if(!readExact(c,header,16,true)){failure=2;break;}
    size_t n=CameraProtocol::frameLength(header);if(!n){failure=3;break;}
    if(!readExact(c,jpegBytes,n)){failure=2;break;}
    decodeStarted=lastYield=millis();
    if(!CameraProtocol::jpeg(jpegBytes,n)||!decoder->openRAM(jpegBytes,n,draw)){failure=3;break;}
    int w=decoder->getWidth(),h=decoder->getHeight(),factor=1;
    while(factor<8&&((w+factor-1)/factor>cameraWidth||(h+factor-1)/factor>cameraHeight))factor*=2;
    if(!CameraFrame::sourceAllowed(w,h)||(w+factor-1)/factor>cameraWidth||(h+factor-1)/factor>cameraHeight){decoder->close();failure=3;break;}
    memset(decoded,0,cameraWidth*cameraHeight*2);decoder->setPixelType(RGB565_LITTLE_ENDIAN);
    int flag=factor==8?JPEG_SCALE_EIGHTH:factor==4?JPEG_SCALE_QUARTER:factor==2?JPEG_SCALE_HALF:0;
    bool ok=decoder->decode(0,0,flag);decoder->close();
    if(ok)ok=CameraFrame::present(decoded,(w+factor-1)/factor,(h+factor-1)/factor,cameraWidth,cameraHeight,CameraFrame::fit(w,h,cameraWidth,cameraHeight),cancelled,[]{vTaskDelay(1);});
    if(!ok){failure=cancelled()?11:3;break;}
    if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<CameraFrame::streamInternalReserve){failure=5;break;}
    publishFrame(w,h);
    XTouchStackMargin::sample(stackPath);
    failures=0;result=6;vTaskDelay(pdMS_TO_TICKS(20));
   }
  }
  c.stop();if(stop)break;result=failure;
  if(failure==3||failure==5||failure==11||++failures>=3)break;
  result=8;if(!waitRetry(failures))break;
 }
 if(frequencyLock){esp_pm_lock_release(frequencyLock);esp_pm_lock_delete(frequencyLock);}
 XTouchStackMargin::sample(stackPath,true);
 memset(code,0,sizeof(code));running=false;vTaskDelete(nullptr);
}
void releaseBuffers(){free(decoded);free(ready);free(shown);free(jpegBytes);if(decoder){decoder->~JPEGDEC();free(decoder);}decoded=shown=ready=nullptr;jpegBytes=nullptr;decoder=nullptr;memset(code,0,sizeof(code));}
void startStream(){
 releaseBuffers();frames=0;connections=0;lastFrame=0;displayed=0;frameWidth=frameHeight=0;lastWidth=lastHeight=0;paused=false;stop=false;result=0;
 strlcpy(serial,xTouchConfig.xTouchSerialNumber,sizeof(serial));
 cloudSource=xtouch_printer_cloud();strlcpy(account,xtouch_printer_account(),sizeof(account));
 const char *model=xtouch_printer_model_at(xtouch_printer_active());
 auto protocol=CameraProtocol::stream(model);
 if(protocol==CameraProtocol::Stream::Unknown)protocol=CameraProtocol::stream(ui_printer_model_code(model));
 if(protocol==CameraProtocol::Stream::Unknown){result=10;return;}
 useRtsps=protocol==CameraProtocol::Stream::Rtsps;
 IPAddress ip;uint8_t bytes[4];bool valid=xtouch_printer_camera_credentials(host,sizeof(host),code,sizeof(code))&&ip.fromString(host);
 for(int i=0;i<4;++i)bytes[i]=ip[i];
 if(!valid||!CameraProtocol::lan(bytes)||strlen(code)!=8){result=4;return;}

 cameraWidth=lv_disp_get_hor_res(nullptr);cameraHeight=lv_disp_get_ver_res(nullptr);
 if(!CameraFrame::memoryAllowed(cameraWidth,cameraHeight,
  heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT),
  heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),useRtsps)){result=5;return;}
 if(!lock)lock=xSemaphoreCreateMutex();
 decoded=(uint16_t*)ps_malloc(cameraWidth*cameraHeight*2);ready=(uint16_t*)ps_malloc(cameraWidth*cameraHeight*2);shown=(uint16_t*)ps_malloc(cameraWidth*cameraHeight*2);
 if(!useRtsps){
  jpegBytes=(uint8_t*)ps_malloc(CameraProtocol::maxFrame);
  void *state=heap_caps_malloc(sizeof(JPEGDEC),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(state)decoder=new(state) JPEGDEC();
 }
 if(!lock||!decoded||!ready||!shown||(!useRtsps&&(!jpegBytes||!decoder))){releaseBuffers();result=5;return;}
 memset(shown,0,cameraWidth*cameraHeight*2);descriptor={};descriptor.header.magic=LV_IMAGE_HEADER_MAGIC;descriptor.header.cf=LV_IMG_CF_TRUE_COLOR;descriptor.header.w=cameraWidth;descriptor.header.stride=cameraWidth*2;descriptor.header.h=cameraHeight;descriptor.data_size=cameraWidth*cameraHeight*2;descriptor.data=(uint8_t*)shown;lv_img_set_src(view.image,&descriptor);
 if(!wifiPowerHeld){previousWifiPower=WiFi.getSleep();wifiPowerHeld=WiFi.setSleep(WIFI_PS_NONE);}
 running=true;if(xTaskCreatePinnedToCore(worker,"bambu-camera",XTouchNetworkBudgets::CameraWorkerStackBytes,nullptr,2,nullptr,0)!=pdPASS){running=false;lv_img_set_src(view.image,nullptr);lv_img_cache_invalidate_src(&descriptor);releaseBuffers();result=5;}
}
}
void xtouch_camera_suspend(){closeView(CloseReason::Suspend);}
bool xtouch_camera_available(){
 char candidateHost[64],candidateCode[9];
 bool valid=xtouch_printer_camera_credentials(candidateHost,sizeof(candidateHost),candidateCode,sizeof(candidateCode));
 memset(candidateCode,0,sizeof(candidateCode));
 IPAddress ip;if(!valid||!ip.fromString(candidateHost))return false;
 uint8_t bytes[4];for(unsigned i=0;i<4;++i)bytes[i]=ip[i];
 const char *model=xtouch_printer_model_at(xtouch_printer_active());
 auto protocol=CameraProtocol::stream(model);
 if(protocol==CameraProtocol::Stream::Unknown)protocol=CameraProtocol::stream(ui_printer_model_code(model));
 return CameraProtocol::lan(bytes)&&protocol!=CameraProtocol::Stream::Unknown;
}
void xtouch_camera_open(){
 if(view.overlay){closeView(CloseReason::User);return;}if(running)return;
 if(!xtouch_camera_available())return;
 lastClose=CloseReason::None;lastLight=millis()-1000;lastResult=-1;lastLanguage=255;wantRestart=true;xtouch_print_preview_suspend(true);cameraUiCreate(view,msg(0),action);
 if(!xtouch_print_preview_status().busy){wantRestart=false;startStream();}
}
void xtouch_camera_poll(){
 static uint32_t availabilityAt=0;static bool available=false;
 if(uint32_t(millis()-availabilityAt)>=250){availabilityAt=millis();bool next=xtouch_camera_available();if(next!=available){available=next;lv_msg_send(XTOUCH_ON_IPCAM,nullptr);}}
 if(view.overlay&&WiFi.status()!=WL_CONNECTED)closeView(CloseReason::Network);
 if(view.overlay&&xtouch_portal_maintenance())closeView(CloseReason::Maintenance);
 if(view.overlay&&serial[0]&&(strcmp(serial,xTouchConfig.xTouchSerialNumber)||cloudSource!=xtouch_printer_cloud()||strcmp(account,xtouch_printer_account())))closeView(CloseReason::Source);
 if(view.overlay&&running){
  char currentHost[64],currentCode[9];
  if(!xtouch_printer_camera_credentials(currentHost,sizeof(currentHost),currentCode,sizeof(currentCode))||strcmp(host,currentHost)||strcmp(code,currentCode))closeView(CloseReason::Credentials);
  memset(currentCode,0,sizeof(currentCode));
 }
 if(view.overlay&&wantRestart&&!running&&!xtouch_print_preview_status().busy){wantRestart=false;lv_img_set_src(view.image,nullptr);lv_img_cache_invalidate_src(&descriptor);startStream();}
 if(view.overlay){
  int state=result,value=paused&&state==6?9:state;
  unsigned w=frameWidth,h=frameHeight,language=xtouch_language_get();
  if(lastResult!=value||lastWidth!=w||lastHeight!=h||lastLanguage!=language){
   char title[128];if((value==6||value==9)&&w&&h)snprintf(title,sizeof(title),"%s %ux%u",msg(value),w,h);else strlcpy(title,msg(value),sizeof(title));
   lv_label_set_text(view.caption,title);lastResult=value;lastWidth=w;lastHeight=h;lastLanguage=language;cameraUiShow(view,true,millis());
  }
  if(!paused&&shown&&ready&&lock&&frames!=displayed&&xSemaphoreTake(lock,0)==pdTRUE){displayed=frames;std::swap(shown,ready);descriptor.data=(uint8_t*)shown;lv_img_cache_invalidate_src(&descriptor);lv_obj_invalidate(view.image);xSemaphoreGive(lock);if(xtouch_screen_onScreenOffTimer)lv_timer_reset(xtouch_screen_onScreenOffTimer);}
  cameraUiTick(view,millis(),state==6,paused,bambuStatus.chamberLed,xtouch_printer_online());
 }
 if(!running&&wifiPowerHeld){WiFi.setSleep(previousWifiPower);wifiPowerHeld=false;}
 if(!view.overlay&&!running){releaseBuffers();serial[0]=0;xtouch_print_preview_suspend(false);}
}

XTouchCameraStatus xtouch_camera_status(){return {view.overlay!=nullptr,bool(running),unsigned(frames),unsigned(connections),unsigned(paused&&result==6?9:int(result)),unsigned(frameWidth),unsigned(frameHeight),lastFrame?uint32_t(millis()-lastFrame):0,heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),unsigned(lastClose)};}

#else
#include "camera.h"
void xtouch_camera_open(){}
bool xtouch_camera_available(){return false;}
void xtouch_camera_suspend(){}
void xtouch_camera_poll(){}
XTouchCameraStatus xtouch_camera_status(){return {};}
#endif
