
#include "ui.h"
#include "detail_page.h"
#include "ui_msgs.h"
#if defined(XTOUCH_ENABLE_SCREEN_PROTECTION) && XTOUCH_ENABLE_SCREEN_PROTECTION
#include "screen_protection.h"

static int8_t protectionX,protectionY;
void ui_screen_protection_offset(int8_t x,int8_t y){
    if(x==protectionX&&y==protectionY)return;
    lv_obj_t *screen=lv_scr_act();
    for(unsigned i=0;i<lv_obj_get_child_cnt(screen);++i){
        lv_obj_t *child=lv_obj_get_child(screen,i);
        lv_obj_set_style_translate_x(child,lv_obj_get_style_translate_x(child,(lv_part_t)(0))-protectionX+x,0);
        lv_obj_set_style_translate_y(child,lv_obj_get_style_translate_y(child,(lv_part_t)(0))-protectionY+y,0);
    }
    protectionX=x;protectionY=y;
}
#endif
static bool printerNoticeActive;
static lv_timer_t *printerNoticeTimer;

bool isPrinterNoticeActive(void)
{
    return printerNoticeActive;
}

static void closePrinterNotice(lv_timer_t *timer)
{
    printerNoticeActive = false;
    printerNoticeTimer = NULL;
    lv_timer_del(timer);
    if (xTouchConfig.currentScreenIndex == 0) fillScreenData(0);
}

void showPrinterNotice(void)
{
    if (printerNoticeTimer) lv_timer_del(printerNoticeTimer);
    printerNoticeActive = true;
    printerNoticeTimer = lv_timer_create(closePrinterNotice, 2000, NULL);
    fillScreenData(0);
}

void sendMqttMsg(int message, uint32_t data)
{
    struct XTOUCH_MESSAGE_DATA eventData;
    eventData.data = data;
    lv_msg_send(message, &eventData);
}

void fillScreenData(int screen)
{
    switch (screen)
    {
    case 0:
        sendMqttMsg(XTOUCH_ON_BED_TEMP, bambuStatus.bed_temper);
        sendMqttMsg(XTOUCH_ON_BED_TARGET_TEMP, bambuStatus.bed_target_temper);
        sendMqttMsg(XTOUCH_ON_NOZZLE_TEMP, bambuStatus.nozzle_temper);
        sendMqttMsg(XTOUCH_ON_NOZZLE_TARGET_TEMP, bambuStatus.nozzle_target_temper);
        sendMqttMsg(XTOUCH_ON_LIGHT_REPORT, bambuStatus.chamberLed);
        sendMqttMsg(XTOUCH_ON_AMS, bambuStatus.ams);
        sendMqttMsg(XTOUCH_ON_PRINT_STATUS, 0);
        sendMqttMsg(XTOUCH_ON_CHAMBER_TEMP, bambuStatus.chamber_temper);
        break;
    case 1:
        sendMqttMsg(XTOUCH_ON_BED_TEMP, bambuStatus.bed_temper);
        sendMqttMsg(XTOUCH_ON_BED_TARGET_TEMP, bambuStatus.bed_target_temper);
        sendMqttMsg(XTOUCH_ON_NOZZLE_TEMP, bambuStatus.nozzle_temper);
        sendMqttMsg(XTOUCH_ON_NOZZLE_TARGET_TEMP, bambuStatus.nozzle_target_temper);
        sendMqttMsg(XTOUCH_ON_PART_FAN_SPEED, bambuStatus.cooling_fan_speed);
        sendMqttMsg(XTOUCH_ON_PART_AUX_SPEED, bambuStatus.big_fan1_speed);
        sendMqttMsg(XTOUCH_ON_PART_CHAMBER_SPEED, bambuStatus.big_fan2_speed);
        break;
    case 2:
        sendMqttMsg(XTOUCH_CONTROL_INC_SWITCH, controlMode.inc);
        break;
    case 3:
        sendMqttMsg(XTOUCH_ON_NOZZLE_TEMP, bambuStatus.nozzle_temper);
        break;
    }
}

void loadScreen(int screen)
{
    if (screen < -1 || screen > 4) return;
    ui_detail_page_navigation();
    xTouchConfig.currentScreenIndex = screen;
 #if defined(XTOUCH_ENABLE_SCREEN_PROTECTION) && XTOUCH_ENABLE_SCREEN_PROTECTION
    protectionX=protectionY=0;
 #endif
    lv_obj_t *current = lv_scr_act();
    /* The LVGL pool cannot hold two full pages. Load a lightweight screen
       without refreshing it, release the old page, then construct the next. */
    lv_obj_t *transition = lv_obj_create(NULL);
    lv_disp_load_scr(transition);
    if (current != NULL) lv_obj_del(current);

    switch (screen)
    {
    case -1:
        ui_introScreen_screen_init();
        lv_disp_load_scr(introScreen);
        break;
    case 0:
        ui_homeScreen_screen_init();
        lv_disp_load_scr(ui_homeScreen);
        break;
    case 1:
        ui_temperatureScreen_screen_init();
        lv_disp_load_scr(ui_temperatureScreen);
        break;
    case 2:
        ui_controlScreen_screen_init();
        lv_disp_load_scr(ui_controlScreen);
        break;
    case 3:
        ui_filamentScreen_screen_init();
        lv_disp_load_scr(ui_filamentScreen);
        break;
    case 4:
        ui_settingsScreen_screen_init();
        lv_disp_load_scr(ui_settingsScreen);
        break;
    }

    lv_obj_del(transition);
    // A page owns its full background; page changes must not retain old pixels.
    lv_obj_set_style_bg_opa(lv_scr_act(),LV_OPA_COVER,0);
    lv_obj_set_style_opa(lv_scr_act(),LV_OPA_COVER,0);
    fillScreenData(screen);

    if (screen >= 0 && screen < 5)
    {
        ui_sidebarComponent_set_active(screen);
    }
    lv_obj_invalidate(lv_scr_act());
}

void initTopLayer()
{
    ui_confirmComponent = ui_confirmPanel_create(lv_layer_top());
    lv_obj_add_flag(ui_confirmComponent, LV_OBJ_FLAG_HIDDEN);
    ui_hmsComponent = ui_hmsPanel_create(lv_layer_top());
    lv_obj_add_flag(ui_hmsComponent, LV_OBJ_FLAG_HIDDEN);
}

/* Persistent dialogs outlive individual pages; rebuild after loading/changing language. */
void ui_refresh_top_layer_language(void)
{
    ui_notice_hide();
    ui_confirmPanel_hide();
    if (ui_confirmComponent) lv_obj_del(ui_confirmComponent);
    if (ui_hmsComponent) lv_obj_del(ui_hmsComponent);
    initTopLayer();
}
