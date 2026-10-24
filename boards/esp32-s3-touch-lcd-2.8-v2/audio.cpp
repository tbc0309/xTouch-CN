#include "audio_output.h"
// Waveshare V2 PCM5101: stereo I2S, no control bus or software PA enable.
BoardAudioPins board_audio_pins(){return {48,38,47,-1,-1};}
bool board_audio_codec_begin(){return true;}
void board_audio_codec_mute(bool){}
