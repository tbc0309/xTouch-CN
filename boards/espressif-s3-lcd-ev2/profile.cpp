#include "port_profile.h"
#include "port_module_pins.h"
#include <esp_psram.h>
namespace XTouchPort {
const Profile &profile() {
    static const Profile p = [] {
        Profile value = {
            Panel::Rgb,    // panel
            800,           // width
            480,           // height
            1,             // spiHost
            12,            // clock
            11,            // mosi
            13,            // miso
            10,            // cs
            46,            // dc
            -1,            // reset
            -1,            // data2
            -1,            // data3
            40000000,      // hz
            0,             // spiMode
            true,          // swap
            false,         // mirrorX
            false,         // mirrorY
            true,          // bgr
            true,          // invert
            false,         // qspi
            -1,            // backlight
            false,         // backlightInvert
            47,            // sda
            48,            // scl
            -1,            // touchReset
            -1,            // touchInterrupt
            Touch::Gt1151, // touch
            20,            // touchAddress
            false,         // touchSwap
            false,         // touchMirrorX
            false,         // touchMirrorY
            16,            // bclk
            7,             // ws
            6,             // dout
            5,             // mclk
            -1,            // pa
            false,         // paActiveHigh
            4,             // indicator
            false,         // indicatorRgb
            32,            // expander
            1,             // expanderOutput
            254,           // expanderDirection
            Power::Gpio,   // power
            Codec::Es8311, // codec
            false,         // customSt7796
        };
        const auto pins = lcdEvModulePins(esp_psram_get_size());
        value.sda = pins.sda;
        value.scl = pins.scl;
        return value;
    }();
    return p;
}
const RgbProfile &rgbProfile() {
    static const RgbProfile r = [] {
        RgbProfile value = {{10, 11, 12, 13, 14, 21, 8, 18, 45, 38, 39, 40, 41, 42, 2, 1},
                            46,
                            3,
                            17,
                            9,
                            40,
                            40,
                            48,
                            23,
                            32,
                            13,
                            true,
                            18000000};
        const auto pins = lcdEvModulePins(esp_psram_get_size());
        value.data[6] = pins.data6;
        value.data[7] = pins.data7;
        return value;
    }();
    return r;
}
} // namespace XTouchPort
