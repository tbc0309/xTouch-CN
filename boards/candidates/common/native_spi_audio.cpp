#include "audio_output.h"
#include "es8311_output.h"
#include "native_spi_config.h"
BoardAudioPins board_audio_pins() {
    const auto &p = NativeCandidate::config();
    return {p.bclk, p.ws, p.dout, p.mclk, p.pa, true};
}
bool board_audio_codec_begin() { return board_es8311_begin(); }
void board_audio_codec_mute(bool muted) { board_es8311_mute(muted); }
