#include "port_profile.h"
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
  v.width = 800;
  v.height = 480;
  v.spiHost = 1;
  v.paActiveHigh = true;
  v.indicatorRgb = true;
  v.codec = Codec::Es8389;
  v.power = Power::Gpio;
  v.panel = Panel::Rgb;
  v.sda = 0;
  v.scl = 1;
  v.touch = Touch::Gt1151;
  v.touchAddress = 20;
  v.bclk = 3;
  v.ws = 4;
  v.dout = 5;
  v.mclk = 2;
  v.pa = 7;
  v.codecAddress = 32;
  v.codecUseMclk = false;
  v.indicator = 37;
  return v;
 }();
 return p;
}
const RgbProfile &rgbProfile() {
 static const RgbProfile r = {{8,9,10,11,12,13,14,15,16,17,18,19,33,34,35,36},44,45,43,40,4,8,8,4,8,8,true,16000000,38};
 return r;
}
}
