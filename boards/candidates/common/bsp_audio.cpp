#include "audio_output.h"
#include <bsp/esp-bsp.h>
#include <esp_codec_dev.h>
#include <limits>

namespace {
esp_codec_dev_handle_t speaker;
bool active;
} // namespace

bool board_audio_begin() {
    if (speaker)
        return true;
    speaker = bsp_audio_codec_speaker_init();
    if (!speaker)
        return false;
    esp_codec_dev_sample_info_t format{};
    format.sample_rate = 16000;
    format.channel = 2;
    format.bits_per_sample = 16;
    if (esp_codec_dev_open(speaker, &format) != ESP_CODEC_DEV_OK ||
        esp_codec_dev_set_out_vol(speaker, 80) != ESP_CODEC_DEV_OK ||
        esp_codec_dev_set_out_mute(speaker, true) != ESP_CODEC_DEV_OK) {
        esp_codec_dev_close(speaker);
        speaker = nullptr;
        return false;
    }
    return true;
}

bool board_audio_active(bool enabled) {
    if (!speaker || esp_codec_dev_set_out_mute(speaker, !enabled) != ESP_CODEC_DEV_OK)
        return false;
    active = enabled;
    return true;
}

bool board_audio_write(const int16_t *samples, size_t count) {
    if (!speaker || !active || !samples || count > size_t(std::numeric_limits<int>::max()) / 2)
        return false;
    return esp_codec_dev_write(speaker, const_cast<int16_t *>(samples), int(count * 2)) ==
           ESP_CODEC_DEV_OK;
}
