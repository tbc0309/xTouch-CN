#include "audio_output.h"
#include "es8311_output.h"
BoardAudioPins board_audio_pins() {
    return {18, 21, 15, 17, 1, false};
}
bool board_audio_codec_begin() {
    return board_es8311_begin();
}
void board_audio_codec_mute(bool muted) {
    board_es8311_mute(muted);
}
