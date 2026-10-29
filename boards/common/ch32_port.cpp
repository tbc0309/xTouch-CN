#include "ch32_port.h"
#include "platform/idf/i2c.h"
#include "platform/idf/runtime.h"
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#if CONFIG_IDF_TARGET_ESP32C5
#include <XPowersAXP2101.hpp>
#endif
namespace {
bool write(uint8_t reg, uint8_t value) {
    const uint8_t bytes[] = {reg,value};
    return xtouch_i2c.transfer(0x24,bytes,2,nullptr,0)==ESP_OK;
}
uint8_t output;
bool outputValue(uint8_t value) {
    if (!write(3,value)) return false;
    output=value;
    return true;
}
void brightness(uint8_t value) { write(5,value); }
void amplifier(bool enabled) {
#if CONFIG_IDF_TARGET_ESP32C5
    const uint8_t bit=32;
#else
    const uint8_t bit=8;
#endif
    outputValue(uint8_t((output & ~bit) | (enabled ? bit : 0)));
}
bool resetExpander(int sd, uint8_t direction) {
    if (gpio_set_direction(gpio_num_t(sd),GPIO_MODE_OUTPUT)!=ESP_OK ||
        gpio_set_level(gpio_num_t(sd),1)!=ESP_OK) return false;
    if (!write(5,0) || !write(2,direction) || !outputValue(3)) return false;
    delay(50);
    if (!outputValue(0)) return false;
    delay(200);
    if (!outputValue(3)) return false;
    delay(200);
    return true;
}
bool c6Begin() { return resetExpander(23,11); }
#if CONFIG_IDF_TARGET_ESP32C5
int pmuRead(uint8_t addr,uint8_t reg,uint8_t *data,uint8_t n) {
 return xtouch_i2c.transfer(addr,&reg,1,data,n)==ESP_OK ? 0 : -1;
}
int pmuWrite(uint8_t addr,uint8_t reg,uint8_t *data,uint8_t n) {
 if (n>32) return -1;
 uint8_t bytes[33]; bytes[0]=reg;
 memcpy(bytes+1,data,n);
 return xtouch_i2c.transfer(addr,bytes,n+1,nullptr,0)==ESP_OK ? 0 : -1;
}
bool c5Begin() {
 static XPowersAXP2101 pmic;
 if (!pmic.begin(0x34,pmuRead,pmuWrite)) return false;
 pmic.disableWatchdog();
 if (!pmic.setDC1Voltage(3300) || !pmic.setDC2Voltage(1000) ||
     !pmic.setDC3Voltage(3300) || !pmic.setDC4Voltage(1000) || !pmic.setDC5Voltage(3300) ||
     !pmic.setALDO1Voltage(3300) || !pmic.setALDO2Voltage(3300) ||
     !pmic.setALDO3Voltage(3300) || !pmic.setALDO4Voltage(1800) ||
     !pmic.setBLDO1Voltage(1500) || !pmic.setBLDO2Voltage(2800) ||
     !pmic.setCPUSLDOVoltage(1000) || !pmic.setDLDO1Voltage(3300)) return false;
 pmic.enableDC1(); pmic.enableDC2(); pmic.enableDC3(); pmic.enableDC4(); pmic.enableDC5();
 pmic.enableALDO1(); pmic.enableALDO2(); pmic.enableALDO3(); pmic.enableALDO4();
 pmic.enableBLDO1(); pmic.enableBLDO2(); pmic.enableCPUSLDO(); pmic.enableDLDO1(); pmic.enableDLDO2();
 pmic.settDC1WorkModeToPwm(true);
 // Preserve the board's battery charging policy; this adapter only enables the display/audio rails.
 if (!resetExpander(9,35)) return false;
 const uint8_t gesture[] = {0xd0,0};
 return xtouch_i2c.transfer(0x38,gesture,2,nullptr,0)==ESP_OK;
}
struct Command { int code; const char *bytes; unsigned count, delayMs; };
const Command commands[] = {
 {0x11,nullptr,0,100},{0x36,"\x08",1,0},{0x3a,"\x55",1,0},{0x11,nullptr,0,120},
 {0x3a,"\x05",1,0},{0xf0,"\xc3",1,0},{0xf0,"\x96",1,0},{0xb4,"\x01",1,0},
 {0xb7,"\xc6",1,0},{0xc0,"\x80\x45",2,0},{0xc1,"\x13",1,0},{0xc2,"\xa7",1,0},
 {0xc5,"\x0a",1,0},{0xe8,"\x40\x8a\x00\x00\x29\x19\xa5\x33",8,0},
 {0xe0,"\xd0\x08\x0f\x06\x06\x33\x30\x33\x47\x17\x13\x13\x2b\x31",14,0},
 {0xe1,"\xd0\x0a\x11\x0b\x09\x07\x2f\x33\x47\x38\x15\x16\x2c\x32",14,0},
 {0xf0,"\x3c",1,0},{0xf0,"\x69",1,120},{0x21,nullptr,0,0},{0x29,nullptr,0,0}
};
bool c5Panel(void *handle) {
 for (const auto &cmd:commands) {
  if (esp_lcd_panel_io_tx_param(static_cast<esp_lcd_panel_io_handle_t>(handle),cmd.code,cmd.bytes,cmd.count)!=ESP_OK) return false;
  if (cmd.delayMs) delay(cmd.delayMs);
 }
 return true;
}
#endif
}
namespace XTouchPort {
const PeripheralHooks c6Peripherals={c6Begin,brightness,amplifier};
#if CONFIG_IDF_TARGET_ESP32C5
const PeripheralHooks c5Peripherals={c5Begin,brightness,amplifier,c5Panel};
#endif
}
