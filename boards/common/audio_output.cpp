#include "features/config.h"
#if XTOUCH_FEATURE_AUDIO
#include "audio_output.h"
#include <Arduino.h>
#include <driver/i2s_std.h>
static i2s_chan_handle_t tx;
static bool active;
bool board_audio_begin(){
 if(tx)return true;
 auto pins=board_audio_pins();if(pins.pa>=0){pinMode(pins.pa,OUTPUT);digitalWrite(pins.pa,LOW);}
 i2s_chan_config_t channel=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,I2S_ROLE_MASTER);channel.dma_desc_num=4;channel.dma_frame_num=128;
 // Do not replay the last DMA buffer when the worker finishes a short tone.
 channel.auto_clear_after_cb=true;
 if(i2s_new_channel(&channel,&tx,nullptr)!=ESP_OK)return false;
 i2s_std_config_t config={};config.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(16000);
#if CONFIG_IDF_TARGET_ESP32P4
 // Keep short UI tones on the crystal clock, independent of the display PLL.
 config.clk_cfg.clk_src=I2S_CLK_SRC_XTAL;
#endif
 config.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO);
 config.gpio_cfg.mclk=(gpio_num_t)pins.mclk;config.gpio_cfg.bclk=(gpio_num_t)pins.bclk;config.gpio_cfg.ws=(gpio_num_t)pins.ws;config.gpio_cfg.dout=(gpio_num_t)pins.dout;config.gpio_cfg.din=GPIO_NUM_NC;
 if(i2s_channel_init_std_mode(tx,&config)!=ESP_OK||i2s_channel_enable(tx)!=ESP_OK){i2s_del_channel(tx);tx=nullptr;return false;}
 active=true;
 if(!board_audio_codec_begin()){i2s_channel_disable(tx);i2s_del_channel(tx);tx=nullptr;active=false;return false;}
 board_audio_active(false);return true;
}
bool board_audio_active(bool enabled){
 if(!tx)return false;if(active==enabled)return true;
 auto pins=board_audio_pins();
 if(enabled){if(i2s_channel_enable(tx)!=ESP_OK)return false;active=true;board_audio_codec_mute(false);if(pins.pa>=0)digitalWrite(pins.pa,HIGH);}
 else {board_audio_codec_mute(true);if(pins.pa>=0)digitalWrite(pins.pa,LOW);i2s_channel_disable(tx);active=false;}
 return true;
}
bool board_audio_write(const int16_t *samples,size_t count){size_t written=0;return tx&&active&&i2s_channel_write(tx,samples,count*sizeof(int16_t),&written,100)==ESP_OK&&written==count*sizeof(int16_t);}
#endif
