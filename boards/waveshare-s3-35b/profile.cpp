#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile p = {
        Panel::Axs15231b, // panel
        480,              // width
        320,              // height
        1,                // spiHost
        5,                // clock
        1,                // mosi
        2,                // miso
        12,               // cs
        -1,               // dc
        -1,               // reset
        3,                // data2
        4,                // data3
        40000000,         // hz
        3,                // spiMode
        true,             // swap
        false,            // mirrorX
        false,            // mirrorY
        true,             // bgr
        true,             // invert
        true,             // qspi
        6,                // backlight
        false,            // backlightInvert
        8,                // sda
        7,                // scl
        -1,               // touchReset
        -1,               // touchInterrupt
        Touch::Axs15231b, // touch
        59,               // touchAddress
        true,             // touchSwap
        true,             // touchMirrorX
        true,             // touchMirrorY
        13,               // bclk
        15,               // ws
        16,               // dout
        44,               // mclk
        -1,               // pa
        false,            // paActiveHigh
        -1,               // indicator
        true,             // indicatorRgb
        -1,               // expander
        0,                // expanderOutput
        255,              // expanderDirection
        Power::Tca9554,   // power
        Codec::Es8311,    // codec
        false,            // customSt7796
    };
    return p;
}
} // namespace XTouchPort
