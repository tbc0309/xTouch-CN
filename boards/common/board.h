#pragma once
#include "platform/idf/runtime.h"
#include "ui/lvgl_compat.h"
bool board_display_begin();
void board_display_ready(
    lv_display_t *display); // Register driver hooks before the first LVGL frame.
bool board_draw_buffers(uint16_t **first, uint16_t **second, uint32_t *pixels);
bool board_pixels_async(lv_display_t *driver, int x, int y, int w, int h, const uint16_t *pixels);
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels);
void board_flush(); // Complete a logical LVGL refresh after its final draw-buffer chunk.
bool board_touch(int &x, int &y);
void board_brightness(uint8_t level);
bool board_display_sleep(bool sleeping);
void board_invert(bool invert);
bool board_card_begin();
void xtouch_screen_setup();
void xtouch_touch_setup();
void xtouch_resetTouchConfig();
void xtouch_screen_setupTFTFlip();
void xtouch_screen_setBrightness(byte);
void xtouch_screen_invertColors();
byte xtouch_screen_getTFTFlip();
void xtouch_screen_setTFTFlip(byte);
void xtouch_screen_toggleTFTFlip();
void xtouch_screen_setupScreenTimer();
void xtouch_screen_startScreenTimer();
void xtouch_screen_setScreenTimer(uint32_t);
void xtouch_screen_wakeUp();
extern bool xtouch_screen_touchFromPowerOff;
