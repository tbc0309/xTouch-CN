#pragma once
#include <stddef.h>
#include <stdint.h>
struct BoardAudioPins {int bclk,ws,dout,mclk,pa;};
BoardAudioPins board_audio_pins();
bool board_audio_codec_begin();
void board_audio_codec_mute(bool mute);
bool board_audio_begin();
bool board_audio_active(bool active);
bool board_audio_write(const int16_t *samples,size_t count);
