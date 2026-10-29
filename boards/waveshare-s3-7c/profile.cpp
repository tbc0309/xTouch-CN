#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile p = {
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
        4,             // touchInterrupt
        Touch::Gt911,  // touch
        93,            // touchAddress
        false,         // touchSwap
        false,         // touchMirrorX
        false,         // touchMirrorY
        44,            // bclk
        16,            // ws
        15,            // dout
        6,             // mclk
        -1,            // pa
        false,         // paActiveHigh
        -1,            // indicator
        true,          // indicatorRgb
        -1,            // expander
        78,            // expanderOutput
        78,            // expanderDirection
        Power::Ch32,   // power
        Codec::Es8389, // codec
        false,         // customSt7796
        16,            // expanderWidth
    };
    return p;
}
const RgbProfile &rgbProfile() {
    static const RgbProfile r = {{14, 38, 18, 17, 10, 39, 0, 45, 9, 8, 21, 1, 2, 42, 41, 40},
                                 46,
                                 3,
                                 5,
                                 7,
                                 8,
                                 8,
                                 4,
                                 8,
                                 8,
                                 4,
                                 true};
    return r;
}
} // namespace XTouchPort
