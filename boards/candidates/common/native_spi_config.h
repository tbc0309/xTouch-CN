#pragma once
namespace NativeCandidate {
enum class Panel { St7789, St7796 };
enum class Touch { Ft6336, Tt21100 };
struct Config {
    Panel panel;
    Touch touch;
    int clock, mosi, miso, cs, dc, reset, backlight;
    int sda, scl, touchAddress;
    int bclk, ws, dout, mclk, pa;
    bool swap, mirrorX, mirrorY, bgr, invert;
    bool touchSwap, touchMirrorX, touchMirrorY;
    bool classicPower;
};
const Config &config();
} // namespace NativeCandidate
