// Display timing and ST7701 commands: Waveshare board examples, Apache-2.0.
#include "board.h"
#include "geometry.h"
#include "framebuffer.h"
#include "st7701_init.h"
#include "startup.h"
#include <Wire.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_cache.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_ldo_regulator.h>
static esp_lcd_panel_handle_t panel;
static esp_lcd_panel_io_handle_t panelIO;
static uint16_t *fb,*front;
static volatile uint32_t completedFrames;
static bool IRAM_ATTR frameComplete(esp_lcd_panel_handle_t,esp_lcd_dpi_panel_event_data_t *,void *){++completedFrames;return false;}
static uint8_t touchAddress;
static bool inverted;
static bool backlightReady;
// Waveshare BSP: active-low GPIO26, 5 kHz, 10-bit PWM on timer 1.
// Use IDF directly: Arduino's full-on special case writes 1024 instead of 1023.
static bool backlight_begin(){
 ledc_timer_config_t timer={};timer.speed_mode=LEDC_LOW_SPEED_MODE;
 timer.duty_resolution=LEDC_TIMER_10_BIT;timer.timer_num=LEDC_TIMER_1;
 timer.freq_hz=LCD43_BACKLIGHT_FREQ;timer.clk_cfg=LEDC_AUTO_CLK;
 if(ledc_timer_config(&timer)!=ESP_OK)return false;
 ledc_channel_config_t channel={};channel.gpio_num=LCD43_BACKLIGHT;
 channel.speed_mode=LEDC_LOW_SPEED_MODE;channel.channel=LEDC_CHANNEL_0;
 channel.intr_type=LEDC_INTR_DISABLE;channel.timer_sel=LEDC_TIMER_1;
 channel.duty=0;channel.hpoint=0;channel.flags.output_invert=1;
 backlightReady=ledc_channel_config(&channel)==ESP_OK;
 return backlightReady;
}
static bool readReg(uint16_t reg,uint8_t *data,size_t n){Wire.beginTransmission(touchAddress);Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));if(Wire.endTransmission(false))return false;if(Wire.requestFrom(touchAddress,n)!=n)return false;while(n--)*data++=Wire.read();return true;}
bool board_display_begin(){
 esp_ldo_channel_handle_t ldo;esp_ldo_channel_config_t power={};power.chan_id=3;power.voltage_mv=2500;if(esp_ldo_acquire_channel(&power,&ldo)!=ESP_OK)return false;
 esp_lcd_dsi_bus_config_t bus={};bus.bus_id=0;bus.num_data_lanes=2;bus.lane_bit_rate_mbps=500;esp_lcd_dsi_bus_handle_t dsi;if(esp_lcd_new_dsi_bus(&bus,&dsi)!=ESP_OK)return false;
 esp_lcd_dbi_io_config_t cmd={};cmd.lcd_cmd_bits=8;cmd.lcd_param_bits=8;esp_lcd_panel_io_handle_t io;if(esp_lcd_new_panel_io_dbi(dsi,&cmd,&io)!=ESP_OK)return false;panelIO=io;
 esp_lcd_dpi_panel_config_t dpi={};dpi.dpi_clk_src=MIPI_DSI_DPI_CLK_SRC_DEFAULT;dpi.dpi_clock_freq_mhz=30;dpi.in_color_format=LCD_COLOR_FMT_RGB565;dpi.out_color_format=LCD_COLOR_FMT_RGB565;dpi.num_fbs=2;dpi.video_timing.h_size=480;dpi.video_timing.v_size=800;dpi.video_timing.hsync_pulse_width=12;dpi.video_timing.hsync_back_porch=42;dpi.video_timing.hsync_front_porch=42;dpi.video_timing.vsync_pulse_width=8;dpi.video_timing.vsync_back_porch=2;dpi.video_timing.vsync_front_porch=60;
 if(esp_lcd_new_panel_dpi(dsi,&dpi,&panel)!=ESP_OK)return false;
 pinMode(27,OUTPUT);digitalWrite(27,LOW);delay(10);digitalWrite(27,HIGH);delay(10);
 for(auto &c:lcd43_st7701_init){if(esp_lcd_panel_io_tx_param(io,c.cmd,c.data,c.data_bytes)!=ESP_OK)return false;if(c.delay_ms)delay(c.delay_ms);}
 ESP_ERROR_CHECK(esp_lcd_panel_init(panel));void *frames[2]={};ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(panel,2,&frames[0],&frames[1]));front=(uint16_t*)frames[0];fb=(uint16_t*)frames[1];memset(front,0,480*800*2);memset(fb,0,480*800*2);
 esp_lcd_dpi_panel_event_callbacks_t callbacks={};callbacks.on_frame_buf_complete=frameComplete;ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(panel,&callbacks,nullptr));if(!backlight_begin())return false;Wire.begin(7,8,100000);Wire.setTimeOut(20);
 for(uint8_t a:{uint8_t(0x5d),uint8_t(0x14)}){Wire.beginTransmission(a);if(!Wire.endTransmission()){touchAddress=a;break;}}return true;
}
static int dirtyFirst=800,dirtyEnd=0;
static bool presentationWaiting,redrawAfterWait;static uint32_t submittedAt;
void board_pixels(int x,int y,int w,int h,const uint16_t *p){
 if(presentationWaiting){redrawAfterWait=true;return;}
 if(w<=0||h<=0||x<0||y<0||x+w>800||y+h>480)return;
 P4Frame::copy(fb,x,y,w,h,p,inverted?0xffff:0);
 if(x<dirtyFirst)dirtyFirst=x;
 if(x+w>dirtyEnd)dirtyEnd=x+w;
}
void board_flush(){
 if(dirtyEnd<=dirtyFirst)return;
 // Draw into the inactive portrait buffer, then let DSI latch it at a frame boundary.
 esp_err_t rc=ESP_OK;
 if(!presentationWaiting){submittedAt=completedFrames;rc=esp_lcd_panel_draw_bitmap(panel,0,dirtyFirst,480,dirtyEnd,fb);presentationWaiting=rc==ESP_OK;}
 uint32_t start=millis();
 // Two completions also cover an interrupt racing the buffer-index update.
 while(rc==ESP_OK&&uint32_t(completedFrames-submittedAt)<2&&uint32_t(millis()-start)<100)vTaskDelay(1);
 if(rc!=ESP_OK||uint32_t(completedFrames-submittedAt)<2){Serial.println("[DISPLAY] frame presentation timeout");return;}
 presentationWaiting=false;
 uint16_t *previous=front;front=fb;fb=previous;
 // Keep untouched pixels identical for the next partial LVGL refresh.
 memcpy(fb+dirtyFirst*480,front+dirtyFirst*480,(dirtyEnd-dirtyFirst)*480*2);
 esp_cache_msync(fb+dirtyFirst*480,(dirtyEnd-dirtyFirst)*480*2,ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_UNALIGNED);
 dirtyFirst=800;dirtyEnd=0;
}
void board_brightness(uint8_t v){
 if(!backlightReady)return;
 const uint32_t duty=(uint32_t)v*((1U<<LCD43_BACKLIGHT_RESOLUTION)-1U)/255U;
 esp_err_t error=ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty);
 if(error==ESP_OK)error=ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
 if(error!=ESP_OK)Serial.printf("[BACKLIGHT] PWM update failed: %s\n",esp_err_to_name(error));
}
void board_backlight_deep_sleep_hold(){
 // Active-low backlight: retain HIGH after LEDC and the HP GPIO domain stop.
 ledc_stop(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,1);
 gpio_set_direction(GPIO_NUM_26,GPIO_MODE_OUTPUT);gpio_set_level(GPIO_NUM_26,1);
 gpio_hold_en(GPIO_NUM_26);
}
void board_backlight_deep_sleep_restore(){
 gpio_set_direction(GPIO_NUM_26,GPIO_MODE_OUTPUT);gpio_set_level(GPIO_NUM_26,1);
 gpio_hold_dis(GPIO_NUM_26);
}
void board_invert(bool value){inverted=value;lv_obj_invalidate(lv_scr_act());}
bool board_touch(int &x,int &y){
 if(redrawAfterWait&&!presentationWaiting){redrawAfterWait=false;lv_obj_invalidate(lv_scr_act());}
 static bool pressed=false;static int lastX,lastY;
 if(!touchAddress)return false;uint8_t status;if(!readReg(0x814e,&status,1)){pressed=false;return false;}
 if(!(status&0x80)){x=lastX;y=lastY;return pressed;}
 uint8_t bytes[8]={};bool ok=(status&15)>0&&(status&15)<=5&&readReg(0x814f,bytes,8);
 Wire.beginTransmission(touchAddress);Wire.write(0x81);Wire.write(0x4e);Wire.write(0);Wire.endTransmission();if(!ok){pressed=false;return false;}
 int nx=bytes[1]|bytes[2]<<8,ny=bytes[3]|bytes[4]<<8;BoardGeometry::Point p;
 pressed=BoardGeometry::p4Touch(nx,ny,p);if(!pressed)return false;x=lastX=p.x;y=lastY=p.y;return true;
}

// DCS display off + sleep in saves panel power; touch controller stays awake.
bool board_display_sleep(bool sleeping){
 if(!panelIO)return false;
 board_flush();
 if(sleeping){if(esp_lcd_panel_io_tx_param(panelIO,0x28,nullptr,0)!=ESP_OK)return false;if(esp_lcd_panel_io_tx_param(panelIO,0x10,nullptr,0)!=ESP_OK)return false;delay(120);}
 else {
  // ST7701's video-mode TCON must recover its complete vendor/RGB565 state.
  // Sleep-out alone can leave the panel scanning with a corrupt pixel format.
  digitalWrite(27,LOW);delay(10);digitalWrite(27,HIGH);delay(120);
  for(auto &c:lcd43_st7701_init){if(esp_lcd_panel_io_tx_param(panelIO,c.cmd,c.data,c.data_bytes)!=ESP_OK)return false;if(c.delay_ms)delay(c.delay_ms);}
  lv_obj_invalidate(lv_scr_act());
 }
 return true;
}
