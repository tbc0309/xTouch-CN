#include "audio_output.h"
#include "es8311_output.h"
#include "port_profile.h"
#include "platform/idf/i2c.h"
#include <esp_codec_dev_defaults.h>
#include <cstring>
namespace {
const audio_codec_if_t *special;
const audio_codec_gpio_if_t *specialGpio;
uint8_t address;
bool opened(const audio_codec_ctrl_if_t *) {
    return true;
}
int readReg(const audio_codec_ctrl_if_t *, int reg, int regLen, void *data, int length) {
    if (regLen != 1 || length < 1 || length > 16)
        return -1;
    uint8_t index = uint8_t(reg);
    return xtouch_i2c.transfer(address, &index, 1, static_cast<uint8_t *>(data), length) == ESP_OK
               ? 0
               : -1;
}
int writeReg(const audio_codec_ctrl_if_t *, int reg, int regLen, void *data, int length) {
    if (regLen != 1 || length < 1 || length > 16)
        return -1;
    uint8_t bytes[17];
    bytes[0] = uint8_t(reg);
    std::memcpy(bytes + 1, data, length);
    return xtouch_i2c.transfer(address, bytes, length + 1, nullptr, 0) == ESP_OK ? 0 : -1;
}
audio_codec_ctrl_if_t control = {nullptr, opened, readReg, writeReg, nullptr};
} // namespace
BoardAudioPins board_audio_pins() {
    const auto &p = XTouchPort::profile();
    return {p.bclk, p.ws, p.dout, p.mclk, p.pa, p.paActiveHigh};
}
bool board_audio_codec_begin() {
    const auto kind = XTouchPort::profile().codec;
    if (kind == XTouchPort::Codec::Es8311)
        return board_es8311_begin();
    if (special)
        return true;
    if (kind == XTouchPort::Codec::Aw88298) {
        address = 0x36;
        aw88298_codec_cfg_t cfg{};
        cfg.ctrl_if = &control;
        if (!specialGpio)
            specialGpio = audio_codec_new_gpio();
        if (!specialGpio)
            return false;
        cfg.gpio_if = specialGpio;
        cfg.reset_pin = -1;
        cfg.hw_gain.pa_voltage = 5.0f;
        cfg.hw_gain.codec_dac_voltage = 3.3f;
        cfg.hw_gain.pa_gain = 1.0f;
        special = aw88298_codec_new(&cfg);
    } else {
        address = 0x10;
        es8389_codec_cfg_t cfg{};
        cfg.ctrl_if = &control;
        cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
        cfg.pa_pin = -1;
        cfg.use_mclk = true;
        cfg.mclk_div = 256;
        special = es8389_codec_new(&cfg);
    }
    if (!special)
        return false;
    esp_codec_dev_sample_info_t format{};
    format.sample_rate = 16000;
    format.channel = 2;
    format.bits_per_sample = 16;
    if (special->set_fs(special, &format) || special->enable(special, true) ||
        special->set_vol(special, 0.0f)) {
        audio_codec_delete_codec_if(special);
        special = nullptr;
        return false;
    }
    special->mute(special, true);
    return true;
}
void board_audio_codec_mute(bool muted) {
    if (special)
        special->mute(special, muted);
    else
        board_es8311_mute(muted);
    XTouchPort::amplifier(!muted);
}
