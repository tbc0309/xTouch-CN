#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile value = {
        Panel::Ili9341, // panel
        320,            // width
        240,            // height
        2,              // spiHost
        12,             // clock
        11,             // mosi
        13,             // miso
        10,             // cs
        46,             // dc
        -1,             // reset
        -1,             // data2
        -1,             // data3
        20000000,       // hz
        0,              // spiMode
        true,           // swap
        false,          // mirrorX
        false,          // mirrorY
        true,           // bgr
        true,           // invert
        false,          // qspi
        45,             // backlight
        false,          // backlightInvert
        16,             // sda
        15,             // scl
        -1,             // touchReset
        -1,             // touchInterrupt
        Touch::Ft6x36,  // touch
        56,             // touchAddress
        true,           // touchSwap
        false,          // touchMirrorX
        false,          // touchMirrorY
        5,              // bclk
        7,              // ws
        8,              // dout
        4,              // mclk
        1,              // pa
        false,          // paActiveHigh
        42,             // indicator
        false,          // indicatorRgb
        -1,             // expander
        0,              // expanderOutput
        255,            // expanderDirection
        Power::Gpio,    // power
        Codec::Es8311,  // codec
        false,          // customSt7796
    };
    return value;
}
} // namespace XTouchPort
