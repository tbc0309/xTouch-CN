#include "port_profile.h"
namespace XTouchPort {
const Profile &profile() {
    static const Profile value = {
        Panel::Ili9341, // panel
        320,            // width
        240,            // height
        2,              // spiHost
        36,             // clock
        37,             // mosi
        -1,             // miso
        3,              // cs
        35,             // dc
        -1,             // reset
        -1,             // data2
        -1,             // data3
        40000000,       // hz
        2,              // spiMode
        false,          // swap
        false,          // mirrorX
        false,          // mirrorY
        true,           // bgr
        true,           // invert
        false,          // qspi
        -1,             // backlight
        false,          // backlightInvert
        12,             // sda
        11,             // scl
        -1,             // touchReset
        -1,             // touchInterrupt
        Touch::Ft6x36,  // touch
        56,             // touchAddress
        false,          // touchSwap
        false,          // touchMirrorX
        false,          // touchMirrorY
        34,             // bclk
        33,             // ws
        13,             // dout
        0,              // mclk
        -1,             // pa
        false,          // paActiveHigh
        -1,             // indicator
        true,           // indicatorRgb
        -1,             // expander
        0,              // expanderOutput
        255,            // expanderDirection
        Power::CoreS3,  // power
        Codec::Aw88298, // codec
        false,          // customSt7796
    };
    return value;
}
} // namespace XTouchPort
