#include "../candidates/common/native_spi_config.h"
namespace NativeCandidate {
const Config &config() {
    static const Config p = {
        Panel::St7789, // panel
        Touch::Tt21100, // touch
        7, // clock
        6, // mosi
        -1, // miso
        5, // cs
        4, // dc
        48, // reset
        45, // backlight
        8, // sda
        18, // scl
        0x24, // touchAddress
        17, // bclk
        47, // ws
        15, // dout
        2, // mclk
        46, // pa
        false, // swap
        true, // mirrorX
        true, // mirrorY
        true, // bgr
        false, // invert
        false, // touchSwap
        true, // touchMirrorX
        false, // touchMirrorY
        false // classicPower
    };
    return p;
}
} // namespace NativeCandidate
#include "../candidates/common/native_spi_display.cpp"
