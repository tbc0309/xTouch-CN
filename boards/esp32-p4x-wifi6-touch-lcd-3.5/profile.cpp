#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile p = {
        Panel::St7796, // panel
        480,           // width
        320,           // height
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
        true,          // swap
        true,          // mirrorX
        false,         // mirrorY
        true,          // bgr
        true,          // invert
        false,         // qspi
        28,            // backlight
        false,         // backlightInvert
        7,             // sda
        8,             // scl
        29,            // touchReset
        50,            // touchInterrupt
        Touch::Ft6x36, // touch
        56,            // touchAddress
        true,          // touchSwap
        false,         // touchMirrorX
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
} // namespace XTouchPort
