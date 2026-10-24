#include "audio_output.h"
#include <Wire.h>
#include <es8311_codec.h>
// Share Arduino's locked I2C bus with touch; do not install a second driver.
static const audio_codec_if_t *codec;
static bool opened(const audio_codec_ctrl_if_t *){return true;}
static int writeReg(const audio_codec_ctrl_if_t *,int reg,int regLen,void *data,int length){
 if(regLen!=1||length<1||length>16)return -1;
 Wire.beginTransmission(0x18);Wire.write((uint8_t)reg);Wire.write((uint8_t*)data,length);return Wire.endTransmission()==0?0:-1;
}
static int readReg(const audio_codec_ctrl_if_t *,int reg,int regLen,void *data,int length){
 if(regLen!=1||length<1||length>16)return -1;
 Wire.beginTransmission(0x18);Wire.write((uint8_t)reg);if(Wire.endTransmission(false))return -1;
 if(Wire.requestFrom(uint8_t(0x18),size_t(length))!=length)return -1;
 for(int i=0;i<length;++i)((uint8_t*)data)[i]=Wire.read();return 0;
}
static audio_codec_ctrl_if_t control={nullptr,opened,readReg,writeReg,nullptr};
BoardAudioPins board_audio_pins(){return {12,10,9,13,53};}
bool board_audio_codec_begin(){
 if(codec)return true;
 es8311_codec_cfg_t config={};config.ctrl_if=&control;config.codec_mode=ESP_CODEC_DEV_WORK_MODE_DAC;config.pa_pin=-1;config.use_mclk=true;config.mclk_div=256;
 codec=es8311_codec_new(&config);if(!codec)return false;
 esp_codec_dev_sample_info_t format={};format.sample_rate=16000;format.channel=2;format.bits_per_sample=16;
 if(codec->set_fs(codec,&format)||codec->enable(codec,true)||codec->set_vol(codec,0.0f)){audio_codec_delete_codec_if(codec);codec=nullptr;return false;}
 codec->mute(codec,true);return true;
}
void board_audio_codec_mute(bool muted){if(codec)codec->mute(codec,muted);}
