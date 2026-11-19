#pragma once
#include "runtime.h"
// Single, externally powered DS18B20 on CYD GPIO22. Conversion remains asynchronous.
class NativeOneWireTemperature {
    gpio_num_t pin=GPIO_NUM_22;
    portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
    void low(){gpio_set_level(pin,0);}
    void release(){gpio_set_level(pin,1);}
    bool reset(){low();delayMicroseconds(480);release();delayMicroseconds(70);bool present=!gpio_get_level(pin);delayMicroseconds(410);return present;}
    void bit(bool one){portENTER_CRITICAL(&lock);low();delayMicroseconds(one?6:60);release();delayMicroseconds(one?64:10);portEXIT_CRITICAL(&lock);}
    bool bit(){portENTER_CRITICAL(&lock);low();delayMicroseconds(3);release();delayMicroseconds(10);bool value=gpio_get_level(pin);delayMicroseconds(53);portEXIT_CRITICAL(&lock);return value;}
    void write(uint8_t value){for(unsigned n=0;n<8;++n){bit(value&1);value>>=1;}}
    uint8_t read(){uint8_t value=0;for(unsigned n=0;n<8;++n)if(bit())value|=1<<n;return value;}
public:
    void begin(){gpio_config_t c{};c.pin_bit_mask=1ULL<<pin;c.mode=GPIO_MODE_INPUT_OUTPUT_OD;c.pull_up_en=GPIO_PULLUP_ENABLE;gpio_config(&c);release();}
    void setWaitForConversion(bool){}
    void requestTemperatures(){if(reset()){write(0xcc);write(0x44);}}
    float getTempCByIndex(unsigned index){
        if(index||!reset())return -127;
        write(0xcc);write(0xbe);uint8_t data[9];for(auto &v:data)v=read();
        uint8_t crc=0;for(unsigned n=0;n<8;++n){uint8_t value=data[n];for(unsigned b=0;b<8;++b){bool mix=(crc^value)&1;crc>>=1;if(mix)crc^=0x8c;value>>=1;}}
        if(crc!=data[8])return -127;
        return int16_t((uint16_t(data[1])<<8)|data[0])/16.f;
    }
};
