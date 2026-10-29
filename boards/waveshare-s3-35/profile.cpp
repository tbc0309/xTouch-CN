#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile value = {
        Panel::St7796, // panel
        480,           // width
        320,           // height
        2,             // spiHost
        5,             // clock
        1,             // mosi
        2,             // miso
        -1,            // cs
        3,             // dc
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
        6,             // backlight
        false,         // backlightInvert
        8,             // sda
        7,             // scl
        -1,            // touchReset
        -1,            // touchInterrupt
        Touch::Ft6x36, // touch
        56,            // touchAddress
        true,          // touchSwap
        true,          // touchMirrorX
        true,          // touchMirrorY
        13,            // bclk
        15,            // ws
        16,            // dout
        12,            // mclk
        -1,            // pa
        false,         // paActiveHigh
        -1,            // indicator
        true,          // indicatorRgb
        -1,            // expander
        0,             // expanderOutput
        255,           // expanderDirection
        Power::Gpio,   // power
        Codec::Es8311, // codec
        true,          // customSt7796
    };
    return value;
}
} // namespace XTouchPort
