#include "port_profile.h"
#include "../common/ch32_port.h"
namespace XTouchPort {
const Profile &profile() {
 static const Profile p = [] {
  Profile v{};
  v.clock = -1;
  v.mosi = -1;
  v.miso = -1;
  v.cs = -1;
  v.dc = -1;
  v.reset = -1;
  v.data2 = -1;
  v.data3 = -1;
  v.backlight = -1;
  v.touchReset = -1;
  v.touchInterrupt = -1;
  v.bclk = -1;
  v.ws = -1;
  v.dout = -1;
  v.mclk = -1;
  v.pa = -1;
  v.indicator = -1;
  v.expander = -1;
  v.width = 320;
  v.height = 240;
  v.spiHost = 1;
  v.paActiveHigh = true;
  v.indicatorRgb = true;
  v.codec = Codec::Es8311;
  v.power = Power::Gpio;
  v.panel = Panel::St7789;
  v.clock = 0;
  v.mosi = 1;
  v.miso = 8;
  v.cs = 11;
  v.dc = 10;
  v.hz = 20000000;
  v.swap = true;
  v.mirrorY = true;
  v.invert = true;
  v.sda = 6;
  v.scl = 7;
  v.touch = Touch::Cst3530;
  v.touchAddress = 88;
  v.touchInterrupt = 18;
  v.touchSwap = true;
  v.touchMirrorY = true;
  v.bclk = 15;
  v.ws = 3;
  v.dout = 2;
  v.codecUseMclk = false;
  v.peripherals = &c6Peripherals;
  v.i2cHz = 100000;
  return v;
 }();
 return p;
}
}
