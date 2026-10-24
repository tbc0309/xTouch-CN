#include "features/config.h"
#if XTOUCH_FEATURE_AUDIO && defined(__XTOUCH_SCREEN_28__) && !defined(XTOUCH_WAVESHARE) && XTOUCH_CYD_AUDIO_I2S
// Optional CYD I2S DAC backend: quiet idle; audible bias transitions remain.
// Enable with -DXTOUCH_CYD_AUDIO_I2S=1 for hardware evaluation.
#include "cyd_output.h"
#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/dac.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <cmath>
#include <soc/gpio_sig_map.h>
#include <esp32-hal-matrix.h>

namespace {
constexpr unsigned rate = 24000, buffers = 6, framesPerBuffer = 256;
bool ready, active;

void groundOutput() {
    dac_i2s_disable();
    dac_output_disable(DAC_CHANNEL_2);
    rtc_gpio_deinit(GPIO_NUM_26);
    gpio_set_level(GPIO_NUM_26, 0);
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);
}

bool writeWords(const uint16_t *data, size_t count) {
    const size_t bytes = count * sizeof(uint16_t);
    size_t sent = 0;
    while (sent < bytes) {
        size_t written = 0;
        if (i2s_write(I2S_NUM_0, reinterpret_cast<const uint8_t *>(data) + sent,
                      bytes - sent, &written, pdMS_TO_TICKS(500)) != ESP_OK || !written) return false;
        sent += written;
    }
    return true;
}

bool park(unsigned frames, uint16_t value = 0x8000) {
    uint16_t mid[256];
    for (auto &word : mid) word = value;
    while (frames) {
        const unsigned count = frames < 128 ? frames : 128;
        if (!writeWords(mid, count * 2)) return false;
        frames -= count;
    }
    return true;
}
bool ramp(bool up) {
    constexpr unsigned frames = rate * 120 / 1000;
    uint16_t words[256];
    for (unsigned pos = 0; pos < frames;) {
        const unsigned n = frames - pos < 128 ? frames - pos : 128;
        for (unsigned i = 0; i < n; ++i) {
            const float fraction = float(pos + i) / float(frames - 1);
            const float gain = (1.f - std::cos(3.14159265f * fraction)) * .5f;
            const uint16_t value = uint16_t(32768.f * (up ? gain : 1.f - gain));
            words[2*i] = words[2*i+1] = value;
        }
        if (!writeWords(words, n * 2)) return false;
        pos += n;
    }
    return true;
}
} // namespace

void board_audio_prepare_idle() {
    static bool prepared = false;
    if (prepared) return;
    prepared = true;
    groundOutput();
}

bool board_audio_begin() {
    if (ready) return true;
    i2s_config_t config = {};
    config.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
    config.sample_rate = rate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = buffers;
    config.dma_buf_len = framesPerBuffer;
    config.tx_desc_auto_clear = false;
    config.use_apll = false;
    if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) return false;
    if (i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN) != ESP_OK) {
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }
    // I2S DAC setup claims both RTC pads. Restore the touch controller's
    // HSPI clock on GPIO25; the only audio output is DAC2 / GPIO26.
    dac_output_disable(DAC_CHANNEL_1);
    rtc_gpio_deinit(GPIO_NUM_25);
    pinMode(25, OUTPUT);
    pinMatrixOutAttach(25, HSPICLK_OUT_IDX, false, false);
    groundOutput();
    if (!park(buffers * framesPerBuffer * 2, 0)) {
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }
    // Local validation probe: measure steady-state DMA throughput using
    // silent samples, not the clock value reported by the driver.
    const uint32_t started = micros();
    const bool ok = park(rate, 0);
    const uint32_t elapsed = micros() - started;
    const unsigned measured = elapsed ? uint64_t(rate) * 1000000 / elapsed : 0;
    Serial.printf("[CYD audio probe] configured=%u measured=%u elapsed_us=%u\n", rate, measured, elapsed);
    if (!ok || measured < rate * 95 / 100 || measured > rate * 105 / 100) {
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }
    i2s_stop(I2S_NUM_0);
    groundOutput();
    ready = true;
    return true;
}

bool board_audio_active(bool enabled) {
    if (!ready) return false;
    if (enabled == active) return true;
    bool ok = true;
    if (enabled) {
        ok = i2s_start(I2S_NUM_0) == ESP_OK && park(buffers * framesPerBuffer * 2, 0);
        if (ok) {
            dac_output_voltage(DAC_CHANNEL_2, 0);
            dac_output_enable(DAC_CHANNEL_2);
            dac_i2s_enable();
            ok = ramp(true);
        }
    } else {
        ok = park(buffers * framesPerBuffer) && ramp(false) && park(buffers * framesPerBuffer * 2, 0);
    }
    if (!enabled || !ok) {
        i2s_stop(I2S_NUM_0);
        groundOutput();
        active = false;
    } else active = true;
    return ok;
}

bool board_audio_write(const int16_t *pcm, size_t count) {
    if (!ready || count > 256 || count % 2) return false;
    uint16_t words[256];
    for (size_t n = 0; n < count; ++n) words[n] = uint16_t(int32_t(pcm[n]) + 32768);
    return writeWords(words, count);
}
#endif
