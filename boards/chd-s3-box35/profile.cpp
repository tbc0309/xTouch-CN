#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile value = {
        Panel::St7796, // panel
        480,           // width
        320,           // height
        1,             // spiHost
        7,             // clock
        6,             // mosi
        -1,            // miso
        5,             // cs
        4,             // dc
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
        47,            // backlight
        false,         // backlightInvert
        8,             // sda
        18,            // scl
        -1,            // touchReset
        -1,            // touchInterrupt
        Touch::FtOrGt, // touch
        56,            // touchAddress
        true,          // touchSwap
        true,          // touchMirrorX
        false,         // touchMirrorY
        17,            // bclk
        45,            // ws
        15,            // dout
        2,             // mclk
        46,            // pa
        true,          // paActiveHigh
        -1,            // indicator
        true,          // indicatorRgb
        -1,            // expander
        0,             // expanderOutput
        255,           // expanderDirection
        Power::Gpio,   // power
        Codec::Es8311, // codec
        false,         // customSt7796
    };
    return value;
}
} // namespace XTouchPort
