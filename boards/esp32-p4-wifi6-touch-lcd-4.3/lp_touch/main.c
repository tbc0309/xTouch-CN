/* The panel has no INT wire. Poll it on the LP core while the HP core sleeps.
 * Software I2C is needed because this board uses SDA7/SCL8, not LP-I2C's pins.
 */
#include <stdbool.h>
#include <stdint.h>
#include "ulp_lp_core_gpio.h"
#include "ulp_lp_core_utils.h"

#define SDA LP_IO_NUM_7
#define SCL LP_IO_NUM_8
volatile uint32_t touch_address;
volatile uint32_t touch_samples;
volatile uint32_t touch_ok;
volatile uint32_t touch_fired;
static bool armed, pressed;
static void wait_bit(void){ulp_lp_core_delay_us(5);}
static void line(lp_io_num_t pin, bool high){ulp_lp_core_gpio_set_level(pin,high);wait_bit();}
static bool clock_high(void){
    line(SCL,true);
    for(unsigned i=0;i<100;i++){if(ulp_lp_core_gpio_get_level(SCL))return true;wait_bit();}
    return false;
}
static bool start(void){line(SDA,true);if(!clock_high())return false;line(SDA,false);line(SCL,false);return true;}
static void stop(void){line(SDA,false);clock_high();line(SDA,true);}
static bool put(uint8_t byte){
    for(unsigned i=0;i<8;i++){line(SDA,(byte&0x80)!=0);if(!clock_high())return false;line(SCL,false);byte<<=1;}
    line(SDA,true);if(!clock_high())return false;
    bool ack=!ulp_lp_core_gpio_get_level(SDA);line(SCL,false);return ack;
}
static bool status_read(uint8_t *value){
    bool ok=start()&&put(touch_address<<1)&&put(0x81)&&put(0x4e)&&start()&&put((touch_address<<1)|1);
    uint8_t data=0;
    if(ok){
        line(SDA,true);
        for(unsigned i=0;i<8;i++){if(!clock_high()){ok=false;break;}data=(data<<1)|ulp_lp_core_gpio_get_level(SDA);line(SCL,false);}
        // NACK the single returned byte.
        line(SDA,true);if(!clock_high())ok=false;line(SCL,false);
    }
    stop();*value=data;return ok;
}
static bool status_clear(void){
    bool ok=start()&&put(touch_address<<1)&&put(0x81)&&put(0x4e)&&put(0);stop();return ok;
}
int main(void){
    for(lp_io_num_t pin=SDA;pin<=SCL;pin++){
        ulp_lp_core_gpio_init(pin);ulp_lp_core_gpio_input_enable(pin);
        ulp_lp_core_gpio_set_output_mode(pin,RTCIO_LL_OUTPUT_OD);
        ulp_lp_core_gpio_set_level(pin,1);ulp_lp_core_gpio_output_enable(pin);
        ulp_lp_core_gpio_pullup_enable(pin);
    }
    uint8_t status=0;bool ok=status_read(&status);
    if(ok&&(status&0x80)){
        unsigned count=status&15;ok=count<=5&&status_clear();
        if(ok)pressed=count>0;
    }
    touch_ok=ok;touch_samples++;
    if(ok){
        if(!pressed)armed=true;
        else if(armed){touch_fired=1;ulp_lp_core_wakeup_main_processor();ulp_lp_core_stop_lp_core();}
    }
    // The SDK halts the LP core until the next 200 ms LP timer tick.
    return 0;
}
