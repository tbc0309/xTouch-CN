#include "features/config.h"
#include "events.h"
#include "volume.h"
#include "xtouch/types.h"
#if XTOUCH_FEATURE_AUDIO
#ifdef XTOUCH_WAVESHARE
#include "audio_output.h"
#else
#include "cyd_output.h"
#endif
#include "platform/idf/runtime.h"
#include <cmath>
#include <algorithm>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
static QueueHandle_t pending;
static std::atomic<bool> sleeping{false},playing{false};
static std::atomic<unsigned> played{0},dropped{0},volume{20};
static std::atomic<int> error{0};
#ifdef XTOUCH_WAVESHARE
static constexpr unsigned audioRate=16000;
#endif
static bool playTone(float frequency,unsigned duration){
#if !defined(XTOUCH_WAVESHARE)
 return board_audio_tone(frequency,duration,volume.load(std::memory_order_relaxed),[]{return sleeping.load();});
#else
 const float amplitude=float(xtouch_sound_peak(volume.load(std::memory_order_relaxed)));
 int16_t samples[256];unsigned total=duration*audioRate/1000,position=0;float phase=0;
 while(position<total&&!sleeping){unsigned n=std::min(128u,total-position);
  for(unsigned i=0;i<n;++i){unsigned t=position+i;float envelope=xtouch_sound_envelope(t,total,audioRate);int16_t v=int16_t(std::sin(phase)*amplitude*envelope);phase+=6.2831853f*frequency/audioRate;samples[2*i]=samples[2*i+1]=v;}
  if(!board_audio_write(samples,n*2))return false;position+=n;
 }return true;
#endif
}
static bool silence(unsigned duration){
#if !defined(XTOUCH_WAVESHARE)
 uint32_t started=millis();while(millis()-started<duration&&!sleeping)vTaskDelay(1);
#else
 int16_t zeros[256]={};for(unsigned n=0;n<duration*audioRate/1000&&!sleeping;n+=128)if(!board_audio_write(zeros,256))return false;
 #endif
 return true;
}
static void worker(void*){
 XTouchSoundEvent event;
 for(;;){if(xQueueReceive(pending,&event,portMAX_DELAY)!=pdTRUE||sleeping)continue;playing=true;
  // Muting must avoid codec/DAC activation as well as waveform generation.
  if(!volume.load(std::memory_order_relaxed)){++played;error=0;playing=false;continue;}
  if(!board_audio_begin()||!board_audio_active(true)){error=-1;playing=false;continue;}
  // Waveshare codecs need settling after unmute; CYD uses sampled DAC tones.
#ifdef XTOUCH_WAVESHARE
  bool ok=silence(120);
#else
  bool ok=true;
#endif
  if(ok&&event==XTOUCH_SOUND_TOUCH)ok=playTone(1800,35);
  else if(ok&&event==XTOUCH_SOUND_WARNING)ok=playTone(880,90)&&silence(70)&&playTone(880,90);
  else if(ok&&event==XTOUCH_SOUND_ERROR)ok=playTone(660,100)&&silence(70)&&playTone(660,100)&&silence(70)&&playTone(660,100);
  else if(ok)ok=playTone(1200,80)&&silence(50)&&playTone(1600,100);
  // Drain queued samples with silence before muting the board output.
#if defined(XTOUCH_WAVESHARE)
  bool drained=silence(40);
#else
  bool drained=true;
#endif
  bool parked=board_audio_active(false);if(ok&&drained&&parked){++played;error=0;}else error=-2;playing=false;
 }
}
static void enqueue(XTouchSoundEvent event){if(!pending||sleeping)return;if(event==XTOUCH_SOUND_TOUCH&&(playing||uxQueueMessagesWaiting(pending))){++dropped;return;}if(xQueueOverwrite(pending,&event)!=pdTRUE)++dropped;}
static bool startWorker(){
 if(pending)return true;
 pending=xQueueCreate(1,sizeof(XTouchSoundEvent));if(!pending){error=-3;return false;}
const auto created=xTaskCreate(worker,"ui-sound",4096,nullptr,1,nullptr);
 if(created!=pdPASS){vQueueDelete(pending);pending=nullptr;error=-4;return false;}
 xtouch_sound_bind(enqueue);return true;
}
void xtouch_sound_begin(){
#ifndef XTOUCH_WAVESHARE
 board_audio_prepare_idle();
#endif
 volume.store(xTouchConfig.xTouchSoundVolume,std::memory_order_relaxed);
 xtouch_sound_preferences(xTouchConfig.xTouchTouchSound,xTouchConfig.xTouchWarningSound);
 if(xTouchConfig.xTouchTouchSound||xTouchConfig.xTouchWarningSound)startWorker();
}
bool xtouch_sound_test(XTouchSoundEvent event){if(sleeping||event>XTOUCH_SOUND_COMPLETE||!startWorker())return false;enqueue(event);return true;}

void xtouch_sound_suspend(bool value){sleeping=value;if(value&&pending)xQueueReset(pending);}
XTouchSoundStatus xtouch_sound_status(){return {true,bool(playing),bool(sleeping),unsigned(played),unsigned(dropped),int(error)};}
#else
void xtouch_sound_begin(){}
bool xtouch_sound_test(XTouchSoundEvent){return false;}
void xtouch_sound_suspend(bool){}
XTouchSoundStatus xtouch_sound_status(){return {};}
#endif
