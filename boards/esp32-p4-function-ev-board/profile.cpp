#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile p = {
        Panel::Mipi,   // panel
        1024,          // width
        600,           // height
        1,             // spiHost
        21,            // clock
        20,            // mosi
        -1,            // miso
        23,            // cs
        26,            // dc
        27,            // reset
        -1,            // data2
        -1,            // data3
        40000000,      // hz
        3,             // spiMode
        false,         // swap
        true,         // mirrorX
        true,         // mirrorY
        false,         // bgr
        false,         // invert
        false,         // qspi
        26,            // backlight
        false,         // backlightInvert
        7,             // sda
        8,             // scl
        -1,            // touchReset
        -1,            // touchInterrupt
        Touch::Gt911,  // touch
        93,            // touchAddress
        false,         // touchSwap
        true,          // touchMirrorX
        true,          // touchMirrorY
        12,            // bclk
        10,            // ws
        9,             // dout
        13,            // mclk
        53,            // pa
        true,          // paActiveHigh
        -1,            // indicator
        false,         // indicatorRgb
        -1,            // expander
        0,             // expanderOutput
        255,           // expanderDirection
        Power::Gpio,   // power
        Codec::Es8311, // codec
        false,         // customSt7796
    };
    return p;
}
const VideoProfile &videoProfile() {
    static const VideoProfile v = {1024, 600, 1000, 52, false};
    return v;
}
} // namespace XTouchPort
