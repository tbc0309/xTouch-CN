#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile value = {
        Panel::St7789, // panel
        320,           // width
        240,           // height
        1,             // spiHost
        41,            // clock
        40,            // mosi
        -1,            // miso
        -1,            // cs
        39,            // dc
        -1,            // reset
        -1,            // data2
        -1,            // data3
        40000000,      // hz
        2,             // spiMode
        true,          // swap
        true,          // mirrorX
        false,         // mirrorY
        false,         // bgr
        true,          // invert
        false,         // qspi
        42,            // backlight
        true,          // backlightInvert
        1,             // sda
        2,             // scl
        -1,            // touchReset
        -1,            // touchInterrupt
        Touch::Gt911,  // touch
        93,            // touchAddress
        true,          // touchSwap
        true,          // touchMirrorX
        false,         // touchMirrorY
        14,            // bclk
        13,            // ws
        45,            // dout
        38,            // mclk
        -1,            // pa
        false,         // paActiveHigh
        43,            // indicator
        true,          // indicatorRgb
        25,            // expander
        2,             // expanderOutput
        248,           // expanderDirection
        Power::Gpio,   // power
        Codec::Es8311, // codec
        false,         // customSt7796
    };
    return value;
}
} // namespace XTouchPort
