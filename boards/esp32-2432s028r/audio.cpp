#include "features/sound/cyd_output.h"
#include "features/sound/volume.h"
#include "platform/idf/runtime.h"
#include <driver/dac_oneshot.h>
#include <driver/gptimer.h>
#include <soc/rtc_io_reg.h>
#include <cmath>
namespace {
gptimer_handle_t timer;
dac_oneshot_handle_t dac;
uint8_t samples[1600];
volatile unsigned position=0,count=0;
portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
bool IRAM_ATTR nextSample(gptimer_handle_t,const gptimer_alarm_event_data_t*,void*){
 portENTER_CRITICAL_ISR(&lock);
 REG_SET_FIELD(RTC_IO_PAD_DAC2_REG,RTC_IO_PDAC2_DAC,position<count?samples[position++]:0);
 portEXIT_CRITICAL_ISR(&lock);return false;
}
void quiet(){if(timer)gptimer_stop(timer);if(dac)dac_oneshot_output_voltage(dac,0);}
}
void board_audio_prepare_idle(){if(!dac){pinMode(26,OUTPUT);digitalWrite(26,LOW);}}
bool board_audio_begin(){
 if(timer)return true;
 dac_oneshot_config_t dc{};dc.chan_id=DAC_CHAN_1;
 if(dac_oneshot_new_channel(&dc,&dac)!=ESP_OK)return false;
 dac_oneshot_output_voltage(dac,0);
 gptimer_config_t cfg{};cfg.clk_src=GPTIMER_CLK_SRC_DEFAULT;cfg.direction=GPTIMER_COUNT_UP;cfg.resolution_hz=16000;
 if(gptimer_new_timer(&cfg,&timer)!=ESP_OK)return false;
 gptimer_event_callbacks_t callbacks{};callbacks.on_alarm=nextSample;
 if(gptimer_register_event_callbacks(timer,&callbacks,nullptr)!=ESP_OK)return false;
 gptimer_alarm_config_t alarm{};alarm.alarm_count=1;alarm.flags.auto_reload_on_alarm=true;
 return gptimer_set_alarm_action(timer,&alarm)==ESP_OK&&gptimer_enable(timer)==ESP_OK;
}
bool board_audio_active(bool enabled){if(!enabled)quiet();return true;}
bool board_audio_write(const int16_t*,size_t){return false;}
bool board_audio_tone(float frequency,unsigned duration,unsigned volume,bool (*cancelled)()){
 if(!volume||!duration)return true;
 if(!timer||duration>100)return false;
 unsigned n=duration*16;float amplitude=float(xtouch_sound_peak(volume))/256.f;
 for(unsigned i=0;i<n;++i){unsigned edge=std::min(i,n-1-i);float envelope=edge>=80?1.f:.5f*(1-std::cos(3.14159265f*edge/80));samples[i]=uint8_t(std::lround(amplitude*envelope*(1-std::cos(6.2831853f*frequency*i/16000))));}
 if(cancelled())return true;
 portENTER_CRITICAL(&lock);position=0;count=n;portEXIT_CRITICAL(&lock);
 if(gptimer_set_raw_count(timer,0)!=ESP_OK||gptimer_start(timer)!=ESP_OK)return false;
 uint32_t started=millis();bool complete=false;
 while(!cancelled()&&millis()-started<duration+100){portENTER_CRITICAL(&lock);complete=position==count;portEXIT_CRITICAL(&lock);if(complete)break;delay(1);}
 quiet();return complete||cancelled();
}
