#include "runtime.h"
static int pwmPins[LEDC_CHANNEL_MAX];
static bool pwmReady;
bool ledcAttach(uint8_t pin,uint32_t frequency,uint8_t bits){
 if(!pwmReady){std::fill(std::begin(pwmPins),std::end(pwmPins),-1);pwmReady=true;}
 int channel=-1;for(int i=0;i<LEDC_CHANNEL_MAX;++i)if(pwmPins[i]<0||pwmPins[i]==pin){channel=i;break;}
 if(channel<0)return false;
 ledc_timer_config_t timer{};timer.speed_mode=LEDC_LOW_SPEED_MODE;timer.timer_num=LEDC_TIMER_0;timer.duty_resolution=ledc_timer_bit_t(bits);timer.freq_hz=frequency;timer.clk_cfg=LEDC_AUTO_CLK;
 if(ledc_timer_config(&timer)!=ESP_OK)return false;
 ledc_channel_config_t cfg{};cfg.gpio_num=pin;cfg.speed_mode=LEDC_LOW_SPEED_MODE;cfg.channel=ledc_channel_t(channel);cfg.timer_sel=LEDC_TIMER_0;
 if(ledc_channel_config(&cfg)!=ESP_OK)return false;pwmPins[channel]=pin;return true;
}
bool ledcWrite(uint8_t pin,uint32_t duty){for(int i=0;i<LEDC_CHANNEL_MAX;++i)if(pwmPins[i]==pin)return ledc_set_duty(LEDC_LOW_SPEED_MODE,ledc_channel_t(i),duty)==ESP_OK&&ledc_update_duty(LEDC_LOW_SPEED_MODE,ledc_channel_t(i))==ESP_OK;return false;}
