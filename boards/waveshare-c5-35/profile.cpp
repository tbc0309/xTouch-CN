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
  v.width = 480;
  v.height = 320;
  v.spiHost = 1;
  v.paActiveHigh = true;
  v.indicatorRgb = true;
  v.codec = Codec::Es8311;
  v.power = Power::Gpio;
  v.panel = Panel::St7796;
  v.clock = 6;
  v.mosi = 7;
  v.miso = 2;
  v.cs = 8;
  v.dc = 5;
  v.hz = 60000000;
  v.swap = true;
  v.mirrorX = false;
  v.mirrorY = true;
  v.bgr = true;
  v.invert = true;
  v.sda = 27;
  v.scl = 26;
  v.touch = Touch::Ft6x36;
  v.touchAddress = 56;
  v.touchInterrupt = 3;
  v.touchSwap = true;
  v.touchMirrorY = true;
  v.bclk = 23;
  v.ws = 10;
  v.dout = 25;
  v.codecUseMclk = false;
  v.peripherals = &c5Peripherals;
  return v;
 }();
 return p;
}
}
