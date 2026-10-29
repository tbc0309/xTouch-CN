#include "port_profile.h"
#include "platform/idf/i2c.h"
#include "platform/idf/runtime.h"
namespace {
uint16_t levels;
SemaphoreHandle_t outputMutex;
bool reg(uint8_t index,uint8_t value) {
 const uint8_t data[]={index,value};
 return xtouch_i2c.transfer(0x59,data,2,nullptr,0)==ESP_OK;
}
bool pins(uint16_t value) {
 const uint8_t data[]={2,uint8_t(value),uint8_t(value>>8)};
 if (xtouch_i2c.transfer(0x59,data,3,nullptr,0)!=ESP_OK) return false;
 levels=value; return true;
}
void update(uint16_t mask, bool high) {
 if (!outputMutex || xSemaphoreTake(outputMutex, portMAX_DELAY)!=pdTRUE) return;
 pins(high ? levels | mask : levels & ~mask);
 xSemaphoreGive(outputMutex);
}
void backlight(uint8_t value) { update(0x100U, value==0); }
void amp(bool value) { update(0x20U, value); }
bool begin() {
 if (!outputMutex) outputMutex=xSemaphoreCreateMutex();
 if (!outputMutex) return false;
 const uint8_t index=0x10; uint8_t id;
 if (xtouch_i2c.transfer(0x59,&index,1,&id,1)!=ESP_OK || id!=0x23) return false;
 if (!reg(0x12,0xff) || !reg(0x13,0xff) || !reg(0x11,0x10) ||
     !reg(4,3) || !reg(5,0) || !pins(0x3910)) return false;
 delay(100);
 if (!pins(levels | 0x8000U)) return false;
 delay(200);
 const uint8_t command[]={0x20,0,0,0x80}; uint8_t info[4];
 return xtouch_i2c.transfer(0x2e,command,4,info,4)==ESP_OK;
}
const XTouchPort::PeripheralHooks hooks={begin,backlight,amp};
}
namespace XTouchPort {
const Profile &profile() {
 static const Profile p=[] {
 Profile v{};
 v.panel = Panel::St7789;
 v.width = 320;
 v.height = 240;
 v.spiHost = 1;
 v.clock = 15;
 v.mosi = 16;
 v.miso = 17;
 v.cs = 47;
 v.dc = 48;
 v.reset = -1;
 v.data2 = -1;
 v.data3 = -1;
 v.hz = 60000000;
 v.swap = true;
 v.mirrorX = true;
 v.mirrorY = false;
 v.invert = true;
 v.backlight = -1;
 v.sda = 3;
 v.scl = 2;
 v.touchReset = -1;
 v.touchInterrupt = -1;
 v.touch = Touch::Chsc5432;
 v.touchAddress = 46;
 v.touchSwap = true;
 v.touchMirrorY = true;
 v.bclk = 38;
 v.ws = 39;
 v.dout = 40;
 v.mclk = 21;
 v.pa = -1;
 v.indicator = -1;
 v.expander = -1;
 v.power = Power::Gpio;
 v.codec = Codec::Es8311;
 v.peripherals = &hooks;
 v.i2cHz = 100000;
 return v;
 }();
 return p;
}
}
