#pragma once
#include <cstdint>
namespace XTouchPort {
enum class Panel { St7789, Ili9341, St7796, Axs15231b, Rgb, Mipi };
enum class Touch { Ft6x36, Gt911, Axs15231b, FtOrGt, Gt1151, Cst3530, Chsc5432 };
enum class Power { Gpio, CoreS3, Tca9554, Ch32, Nano };
enum class Codec { Es8311, Aw88298, Es8389 };
struct PeripheralHooks {
    bool (*begin)();
    void (*brightness)(uint8_t);
    void (*amplifier)(bool);
    bool (*panelInit)(void *) = nullptr;
};
struct Profile {
    Panel panel;
    int width, height;
    int spiHost, clock, mosi, miso, cs, dc, reset, data2, data3, hz, spiMode;
    bool swap, mirrorX, mirrorY, bgr, invert, qspi;
    int backlight;
    bool backlightInvert;
    int sda, scl, touchReset, touchInterrupt;
    Touch touch;
    uint8_t touchAddress;
    bool touchSwap, touchMirrorX, touchMirrorY;
    int bclk, ws, dout, mclk, pa;
    bool paActiveHigh;
    int indicator;
    bool indicatorRgb;
    int expander, expanderOutput, expanderDirection;
    Power power = Power::Gpio;
    Codec codec = Codec::Es8311;
    bool customSt7796 = false;
    unsigned expanderWidth = 8;
    const PeripheralHooks *peripherals = nullptr;
    uint8_t codecAddress = 0;
    bool codecUseMclk = true;
    uint32_t i2cHz = 400000;
};
const Profile &profile();
bool preparePeripherals();
bool prepareBacklight();
void amplifier(bool enabled);
struct RgbProfile {
    int data[16];
    int hsync, vsync, de, clock;
    int hPulse, hBack, hFront, vPulse, vBack, vFront;
    bool negativeClock;
    unsigned pixelClockHz = 16000000;
    int displayEnable = -1;
};
const RgbProfile &rgbProfile();
struct VideoProfile {
    int width, height, laneMbps, clockMHz;
    bool portrait;
};
const VideoProfile &videoProfile();
} // namespace XTouchPort
