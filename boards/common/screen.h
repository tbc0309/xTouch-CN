#include "features/sound/events.h"
#pragma once
#include "board.h"
#include "touch_gate.h"
#include "xtouch/power.h"
#include "camera.h"
#ifndef XTOUCH_REFRESH_MS
#define XTOUCH_REFRESH_MS 16
#endif
#ifndef XTOUCH_TOUCH_MS
#define XTOUCH_TOUCH_MS 15
#endif
bool xtouch_screen_touchFromPowerOff=false;
static constexpr int wsWidth=XTOUCH_DISPLAY_WIDTH,wsHeight=XTOUCH_DISPLAY_HEIGHT;
static bool wsFlip=false;
static TouchWakeGate wakeGate;
static int appliedBrightness=-1;
int xtouch_screen_getBrightness(){return appliedBrightness;}
void xtouch_screen_setBrightness(byte value){if(appliedBrightness!=value){board_brightness(value);appliedBrightness=value;}}
void xtouch_screen_invertColors(){board_invert(xTouchConfig.xTouchTFTInvert);}
byte xtouch_screen_getTFTFlip(){return xtouch_hardware_get_flip();}
void xtouch_screen_setTFTFlip(byte value){xtouch_hardware_set_flip(value);xTouchConfig.xTouchTFTFlip=value;wsFlip=value;}
#if defined(XTOUCH_BOARD_CYD)
void board_touch_setup();
void xtouch_resetTouchConfig(){xtouch_storage_delete(xtouch_paths_touch);ESP.restart();}
void xtouch_touch_setup(){board_touch_setup();}
#else
void xtouch_resetTouchConfig(){} // Factory capacitive coordinates need no resistive calibration.
void xtouch_touch_setup(){}
#endif
void xtouch_screen_setupTFTFlip(){wsFlip=xtouch_screen_getTFTFlip();lv_obj_invalidate(lv_scr_act());}
void xtouch_screen_toggleTFTFlip(){xtouch_screen_setTFTFlip(!wsFlip);lv_obj_invalidate(lv_scr_act());}
void xtouch_screen_wakeUp(){lv_disp_trig_activity(NULL);if(xtouch_screen_touchFromPowerOff){xtouch_power_screen(false);board_display_sleep(false);lv_timer_resume(lv_display_get_refr_timer(lv_disp_get_default()));lv_obj_invalidate(lv_scr_act());}xtouch_screen_touchFromPowerOff=false;loadScreen(0);if(xtouch_screen_onScreenOffTimer)lv_timer_reset(xtouch_screen_onScreenOffTimer);xtouch_screen_setBrightness(xTouchConfig.xTouchBacklightLevel);}
void xtouch_screen_sleep(){
 if(xtouch_screen_touchFromPowerOff)return;
 xtouch_camera_suspend();xtouch_screen_setBrightness(0);
 if(!board_display_sleep(true))Serial.println("[POWER] panel sleep command failed");
 xtouch_screen_touchFromPowerOff=true;lv_timer_pause(lv_display_get_refr_timer(lv_disp_get_default()));xtouch_power_screen(true);
}
static void wsSleep(lv_timer_t *timer){auto camera=xtouch_camera_status();if(camera.open&&camera.running&&camera.state==6&&camera.frames&&camera.lastFrameAgeMs<5000){lv_timer_reset(timer);return;}if(xTouchConfig.xTouchTFTOFFValue>=XTOUCH_LCD_MIN_SLEEP_TIME)xtouch_screen_sleep();}
void xtouch_screen_setupScreenTimer(){xtouch_screen_onScreenOffTimer=lv_timer_create(wsSleep,xTouchConfig.xTouchTFTOFFValue*60000,nullptr);lv_timer_pause(xtouch_screen_onScreenOffTimer);}
void xtouch_screen_startScreenTimer(){if(xtouch_screen_onScreenOffTimer)lv_timer_resume(xtouch_screen_onScreenOffTimer);}
void xtouch_screen_setScreenTimer(uint32_t period){if(xtouch_screen_onScreenOffTimer)lv_timer_set_period(xtouch_screen_onScreenOffTimer,period);}
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION && !defined(XTOUCH_BOARD_CYD)
#include "../tests/ui/runtime_tap.h"
static uint16_t *xtouch_test_frame_pixels=nullptr;
bool xtouch_test_frame_prepare(){
 if(!xtouch_test_frame_pixels)xtouch_test_frame_pixels=(uint16_t*)heap_caps_malloc(wsWidth*wsHeight*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 return xtouch_test_frame_pixels!=nullptr;
}
const uint8_t *xtouch_test_frame(){return (const uint8_t*)xtouch_test_frame_pixels;}
#endif
static void wsFlush(lv_display_t *d,const lv_area_t *a,uint8_t *data){
 uint16_t *pixels=reinterpret_cast<uint16_t*>(data);
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION && !defined(XTOUCH_BOARD_CYD)
 if(xtouch_test_frame_pixels)for(int y=a->y1;y<=a->y2;++y)memcpy(xtouch_test_frame_pixels+y*wsWidth+a->x1,pixels+(y-a->y1)*(a->x2-a->x1+1),(a->x2-a->x1+1)*2);
#endif

 if(xtouch_screen_touchFromPowerOff){lv_disp_flush_ready(d);return;}
 int x=a->x1,y=a->y1,w=a->x2-a->x1+1,h=a->y2-a->y1+1;
 if(wsFlip){
  // Reverse a whole tile in place, retaining one DMA submission per tile.
  // Per-line SPI writes made rotated scrolling considerably slower.
  for(int left=0,right=w*h-1;left<right;++left,--right){uint16_t c=pixels[left];pixels[left]=pixels[right];pixels[right]=c;}
  x=wsWidth-a->x2-1;y=wsHeight-a->y2-1;
 }
 if(board_pixels_async(d,x,y,w,h,(uint16_t*)pixels))return;
 board_pixels(x,y,w,h,(uint16_t*)pixels);
 if(lv_disp_flush_is_last(d))board_flush();
 lv_disp_flush_ready(d);
}
static void wsRead(lv_indev_t *,lv_indev_data_t *d){
 int x,y;d->state=LV_INDEV_STATE_REL;bool pressed=board_touch(x,y),asleep=xtouch_screen_touchFromPowerOff;bool accepted=wakeGate.accept(pressed,asleep);if(!pressed)return;
 if(xtouch_screen_onScreenOffTimer)lv_timer_reset(xtouch_screen_onScreenOffTimer);
 if(asleep&&!wakeGate.began)return;
 if(asleep)xtouch_screen_wakeUp();
 if(!accepted)return;
 if(wakeGate.began)xtouch_sound_event(XTOUCH_SOUND_TOUCH,millis());
 d->state=LV_INDEV_STATE_PR;d->point.x=wsFlip?wsWidth-1-x:x;d->point.y=wsFlip?wsHeight-1-y:y;
}
void xtouch_screen_setup(){
 if(xtouch_power_woke_from_touch())wakeGate.suppressUntilRelease();
#if !defined(XTOUCH_BOARD_CYD)
 if(!psramFound()){Serial.println("[BOARD] PSRAM required");while(true)delay(1000);}
#endif
 lv_init();ui_lvgl_draw_buffers_init();lv_tick_set_cb([]()->uint32_t{return millis();});if(!board_display_begin()){Serial.println("[BOARD] Display initialization failed");while(true)delay(1000);}
 uint16_t *pixels=nullptr,*second=nullptr;uint32_t count=wsWidth*24;
 if(!board_draw_buffers(&pixels,&second,&count)){
#if XTOUCH_DRAW_BUFFER_INTERNAL
 pixels=(uint16_t*)heap_caps_malloc(wsWidth*24*sizeof(uint16_t),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
#endif
 if(!pixels)pixels=(uint16_t*)heap_caps_malloc(wsWidth*24*sizeof(uint16_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);assert(pixels);
 }
 auto *display=lv_display_create(wsWidth,wsHeight);
 lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,pixels,second,count*sizeof(uint16_t),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(display,wsFlush);
 board_display_ready(display);
 lv_timer_set_period(lv_display_get_refr_timer(display),XTOUCH_REFRESH_MS);
 auto *touch=lv_indev_create();lv_indev_set_type(touch,LV_INDEV_TYPE_POINTER);lv_indev_set_long_press_time(touch,700);lv_indev_set_read_cb(touch,wsRead);lv_indev_set_display(touch,display);lv_timer_set_period(lv_indev_get_read_timer(touch),XTOUCH_TOUCH_MS);
 LV_EVENT_GET_COMP_CHILD=lv_event_register_id();lv_disp_set_theme(display,lv_theme_default_init(display,lv_color_hex(0x555555),lv_color_hex(0xFF0000),true,LV_FONT_DEFAULT));initTopLayer();board_brightness(255);
}
