#pragma once
#include <stddef.h>
#include <stdint.h>
void board_audio_prepare_idle();
bool board_audio_begin();
bool board_audio_active(bool enabled);
bool board_audio_write(const int16_t *pcm, size_t count);
bool board_audio_tone(float frequency,unsigned duration,unsigned volume,bool (*cancelled)());
