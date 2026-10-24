#include "features/config.h"
#if XTOUCH_FEATURE_AUDIO && defined(__XTOUCH_SCREEN_28__) && !defined(XTOUCH_WAVESHARE) && !XTOUCH_CYD_AUDIO_I2S
#include "cyd_output.h"
#include "volume.h"
#include <Arduino.h>
#include <cmath>
#include <driver/dac.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <soc/rtc_io_reg.h>

namespace {
constexpr unsigned sampleRate = 16000;
constexpr unsigned maxDuration = 100;
constexpr unsigned capacity = sampleRate * maxDuration / 1000;
constexpr unsigned fadeSamples = sampleRate * 5 / 1000;
constexpr gpio_num_t audioPin = GPIO_NUM_26;
hw_timer_t *sampleTimer;
portMUX_TYPE sampleLock = portMUX_INITIALIZER_UNLOCKED;
uint8_t samples[capacity];
volatile unsigned position, sampleCount;

void IRAM_ATTR outputSample() {
    portENTER_CRITICAL_ISR(&sampleLock);
    const uint8_t sample = position < sampleCount ? samples[position++] : 0;
    // DAC2 only: GPIO25 belongs to the touch controller. This register write
    // stays in IRAM-safe code; waveform generation happens in the audio task.
    REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, sample);
    portEXIT_CRITICAL_ISR(&sampleLock);
}

void quietOutput() {
    if (sampleTimer) timerAlarmDisable(sampleTimer);
    dac_output_voltage(DAC_CHANNEL_2, 0);
    dac_output_disable(DAC_CHANNEL_2);
    rtc_gpio_deinit(audioPin);
    gpio_set_level(audioPin, 0);
    gpio_set_direction(audioPin, GPIO_MODE_OUTPUT);
}
} // namespace

bool board_audio_begin() {
    if (sampleTimer) return true;
    // A general-purpose timer, independent of the backlight LEDC peripheral.
    sampleTimer = timerBegin(0, 5, true);
    if (!sampleTimer) return false;
    timerAttachInterrupt(sampleTimer, outputSample, true);
    timerAlarmWrite(sampleTimer, 1000, true); // 80 MHz / 5 / 1000 = 16 kHz.
    dac_cw_generator_disable();
    dac_i2s_disable();
    quietOutput();
    return true;
}

bool board_audio_active(bool enabled) {
    if (!enabled) quietOutput();
    return true;
}

bool board_audio_tone(float frequency, unsigned duration, unsigned volume,
                      bool (*cancelled)()) {
    if (!volume || !duration) return true;
    if (!sampleTimer || duration > maxDuration) return false;
    const unsigned count = duration * sampleRate / 1000;
    const float amplitude = float(xtouch_sound_peak(volume)) / 256.f;
    for (unsigned n = 0; n < count; ++n) {
        const unsigned edge = n < count - 1 - n ? n : count - 1 - n;
        const float envelope = edge >= fadeSamples ? 1.f :
            .5f * (1.f - std::cos(3.14159265f * edge / fadeSamples));
        // Fade both signal and DC bias from/to zero. Idle stays grounded;
        // there is no fixed midpoint to switch on before each short tone.
        const float phase = 6.2831853f * frequency * n / sampleRate;
        samples[n] = uint8_t(std::lround(amplitude * envelope * (1.f - std::cos(phase))));
    }
    if (cancelled()) return true;
    dac_output_voltage(DAC_CHANNEL_2, 0);
    if (dac_output_enable(DAC_CHANNEL_2) != ESP_OK) {
        quietOutput();
        return false;
    }
    portENTER_CRITICAL(&sampleLock);
    position = 0;
    sampleCount = count;
    portEXIT_CRITICAL(&sampleLock);
    timerWrite(sampleTimer, 0);
    timerAlarmEnable(sampleTimer);
    const uint32_t started = millis();
    bool complete = false;
    while (!cancelled() && millis() - started < duration + 100) {
        portENTER_CRITICAL(&sampleLock);
        complete = position == sampleCount;
        portEXIT_CRITICAL(&sampleLock);
        if (complete) break;
        vTaskDelay(1);
    }
    quietOutput();
    return complete || cancelled();
}
#endif
