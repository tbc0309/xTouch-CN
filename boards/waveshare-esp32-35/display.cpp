#include "../candidates/common/native_spi_config.h"
namespace NativeCandidate {
const Config &config() {
    static const Config p = {
        Panel::St7796, // panel
        Touch::Ft6336, // touch
        18, // clock
        23, // mosi
        19, // miso
        5, // cs
        27, // dc
        -1, // reset
        25, // backlight
        21, // sda
        22, // scl
        0x38, // touchAddress
        2, // bclk
        4, // ws
        12, // dout
        3, // mclk
        -1, // pa
        true, // swap
        true, // mirrorX
        true, // mirrorY
        true, // bgr
        true, // invert
        true, // touchSwap
        false, // touchMirrorX
        true, // touchMirrorY
        true // classicPower
    };
    return p;
}
} // namespace NativeCandidate
#include "../candidates/common/native_spi_display.cpp"
