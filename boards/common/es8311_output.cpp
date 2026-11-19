#include "es8311_output.h"
#include "platform/idf/i2c.h"
#include <es8311_codec.h>
#include <cstring>
// Share the transaction-locked I2C bus with touch; do not install a second driver.
static const audio_codec_if_t *codec;
static bool opened(const audio_codec_ctrl_if_t *) {
    return true;
}
static int writeReg(const audio_codec_ctrl_if_t *, int reg, int regLen, void *data, int length) {
    if (regLen != 1 || length < 1 || length > 16)
        return -1;
    uint8_t bytes[17];
    bytes[0] = uint8_t(reg);
    std::memcpy(bytes + 1, data, length);
    return xtouch_i2c.transfer(0x18, bytes, length + 1, nullptr, 0) == ESP_OK ? 0 : -1;
}
static int readReg(const audio_codec_ctrl_if_t *, int reg, int regLen, void *data, int length) {
    if (regLen != 1 || length < 1 || length > 16)
        return -1;
    const uint8_t address = uint8_t(reg);
    return xtouch_i2c.transfer(0x18, &address, 1, static_cast<uint8_t *>(data), length) == ESP_OK
               ? 0
               : -1;
}
static audio_codec_ctrl_if_t control = {nullptr, opened, readReg, writeReg, nullptr};
bool board_es8311_begin() {
    if (codec)
        return true;
    es8311_codec_cfg_t config = {};
    config.ctrl_if = &control;
    config.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    config.pa_pin = -1;
    config.use_mclk = true;
    config.mclk_div = 256;
    codec = es8311_codec_new(&config);
    if (!codec)
        return false;
    esp_codec_dev_sample_info_t format = {};
    format.sample_rate = 16000;
    format.channel = 2;
    format.bits_per_sample = 16;
    if (codec->set_fs(codec, &format) || codec->enable(codec, true) ||
        codec->set_vol(codec, 0.0f)) {
        audio_codec_delete_codec_if(codec);
        codec = nullptr;
        return false;
    }
    codec->mute(codec, true);
    return true;
}
void board_es8311_mute(bool muted) {
    if (codec)
        codec->mute(codec, muted);
}
