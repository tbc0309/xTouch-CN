#include "features/sound/events.h"
#ifndef _XLCD_SCREEN
#define _XLCD_SCREEN

#include <TFT_eSPI.h>
#include "xtouch/power.h"
#include "xtouch/touch_wake.h"
static TouchWakeGate cydWakeGate;

#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS 5

#define LCD_BACK_LIGHT_PIN 21

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT 12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 320;

static lv_disp_draw_buf_t draw_buf;
// Restore the normal draw buffer now that CYD no longer fetches task previews.
static lv_color_t buf[4096];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#include "ui/ui.h"
#include "touch.h"
#include "xtouch/globals.h"

bool xtouch_screen_touchFromPowerOff = false;

void xtouch_screen_ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255)
{
    // Multiply before dividing so full brightness reaches a constant HIGH.
    uint32_t duty = valueMax ? (4095u * min(value, valueMax)) / valueMax : 0;

    // write duty to LEDC
    ledcWrite(channel, duty);
}

static int appliedBrightness=-1;
int xtouch_screen_getBrightness(){return appliedBrightness;}
void xtouch_screen_setBrightness(byte brightness)
{
    if(appliedBrightness==brightness)return;
    appliedBrightness=brightness;
    xtouch_screen_ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
}

void xtouch_screen_setBackLedOff()
{
    pinMode(4, OUTPUT);
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    digitalWrite(4, HIGH);
    digitalWrite(16, HIGH);
    digitalWrite(17, HIGH); // The LEDs are "active low", meaning HIGH == off, LOW == on
}

void xtouch_screen_wakeUp()
{
    lv_disp_trig_activity(NULL);
    if(xtouch_screen_touchFromPowerOff){xtouch_power_screen(false);tft.writecommand(0x11);delay(120);tft.writecommand(0x29);lv_timer_resume(_lv_disp_get_refr_timer(lv_disp_get_default()));lv_obj_invalidate(lv_scr_act());}
    lv_timer_reset(xtouch_screen_onScreenOffTimer);
    xtouch_screen_touchFromPowerOff = false;
    loadScreen(0);
    xtouch_screen_setBrightness(xTouchConfig.xTouchBacklightLevel);
}

void xtouch_screen_sleep(){
    if(xtouch_screen_touchFromPowerOff)return;
    xtouch_screen_setBrightness(0);tft.writecommand(0x28);tft.writecommand(0x10);delay(120);
    xtouch_screen_touchFromPowerOff=true;
    lv_timer_pause(_lv_disp_get_refr_timer(lv_disp_get_default()));xtouch_power_screen(true);
}
void xtouch_screen_onScreenOff(lv_timer_t *){
    if(xTouchConfig.xTouchTFTOFFValue>=XTOUCH_LCD_MIN_SLEEP_TIME)xtouch_screen_sleep();
}

void xtouch_screen_setupScreenTimer()
{
    xtouch_screen_onScreenOffTimer = lv_timer_create(xtouch_screen_onScreenOff, xTouchConfig.xTouchTFTOFFValue * 1000 * 60, NULL);
    lv_timer_pause(xtouch_screen_onScreenOffTimer);
}

void xtouch_screen_startScreenTimer()
{
    lv_timer_resume(xtouch_screen_onScreenOffTimer);
}

void xtouch_screen_setScreenTimer(uint32_t period)
{
    lv_timer_set_period(xtouch_screen_onScreenOffTimer, period);
}

void xtouch_screen_invertColors()
{
    tft.invertDisplay(xTouchConfig.xTouchTFTInvert);
}

byte xtouch_screen_getTFTFlip()
{
    byte val = xtouch_hardware_get_flip();
    xTouchConfig.xTouchTFTFlip = val;
    return val;
}

void xtouch_screen_setTFTFlip(byte mode)
{
    xTouchConfig.xTouchTFTFlip = mode;
    xtouch_hardware_set_flip( mode);
}

void xtouch_screen_toggleTFTFlip()
{
    xtouch_screen_setTFTFlip(!xtouch_screen_getTFTFlip());
    xtouch_resetTouchConfig();
}

void xtouch_screen_setupTFTFlip()
{
    byte eepromTFTFlip = xtouch_screen_getTFTFlip();
    tft.setRotation(eepromTFTFlip == 1 ? 3 : 1);
    x_touch_touchScreen.setRotation(eepromTFTFlip == 1 ? 3 : 1);
}

void xtouch_screen_dispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    if(xtouch_screen_touchFromPowerOff){lv_disp_flush_ready(disp);return;}
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void xtouch_screen_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

    data->state=LV_INDEV_STATE_REL;
    bool pressed=x_touch_touchScreen.tirqTouched()&&x_touch_touchScreen.touched();
    bool accepted=cydWakeGate.accept(pressed,xtouch_screen_touchFromPowerOff);
    if (pressed)
    {
        if(xtouch_screen_onScreenOffTimer)lv_timer_reset(xtouch_screen_onScreenOffTimer);
        if(xtouch_screen_touchFromPowerOff)xtouch_screen_wakeUp();
        // Swallow the entire wake gesture without blocking the loop/network.
        if(!accepted)return;
        if(cydWakeGate.began)xtouch_sound_event(XTOUCH_SOUND_TOUCH,millis());

        ScreenPoint sp = ScreenPoint();
        TS_Point p = x_touch_touchScreen.getPoint();
        sp = getScreenCoords(p.x, p.y);
        data->state = LV_INDEV_STATE_PR;
        data->point.x = sp.x;
        data->point.y = sp.y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void xtouch_screen_setup()
{
    if(xtouch_power_woke_from_touch())cydWakeGate.suppressUntilRelease();

    ConsoleInfo.println("[XTouch][SCREEN] Setup");
    pinMode(XPT2046_CS, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);

    digitalWrite(XPT2046_CS, HIGH); // Touch controller chip select (if used)
    digitalWrite(TFT_CS, HIGH);     // TFT screen chip select
    digitalWrite(SD_CS, HIGH);      // SD card chips select, must use GPIO 5 (ESP32 SS)

    xtouch_screen_setBackLedOff();

    lv_init();

    tft.init();

    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
    ledcAttachPin(LCD_BACK_LIGHT_PIN, LEDC_CHANNEL_0);

    x_touch_spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    x_touch_touchScreen.begin(x_touch_spi);

    xtouch_screen_setupTFTFlip();

    xtouch_screen_setBrightness(255);

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, sizeof(buf)/sizeof(buf[0]));

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenHeight;
    disp_drv.ver_res = screenWidth;
    disp_drv.flush_cb = xtouch_screen_dispFlush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    // Keep bounded tiles on boards without PSRAM; improve scheduling latency.
    lv_timer_set_period(_lv_disp_get_refr_timer(lv_disp_get_default()),25);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.long_press_time = 700; // Deliberate hold opens the device picker.
    indev_drv.read_cb = xtouch_screen_touchRead;
    lv_indev_t *touch=lv_indev_drv_register(&indev_drv);
    lv_timer_set_period(touch->driver->read_timer,15);

    /*Initialize the graphics library */
    LV_EVENT_GET_COMP_CHILD = lv_event_register_id();

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(0x555555), lv_color_hex(0xFF0000), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    initTopLayer();
}

#endif
