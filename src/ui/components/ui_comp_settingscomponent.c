#include "xtouch/ota_request.h"
#include "features/config.h"
#include "xtouch/portal.h"
static void portal_manage_event(lv_event_t *e){(void)e;xtouch_portal_show_management();}
#if XTOUCH_FEATURE_BLE_PROVISIONING
static void portal_ble_event(lv_event_t *e){(void)e;xtouch_portal_request_bluetooth();}
#endif
static void portal_wifi_event(lv_event_t *e){(void)e;xtouch_portal_request_wifi();}
#include <stdio.h>
#include "../ui.h"
#include "../ui_msgs.h"
#include "../settings_gesture.h"
LV_IMG_DECLARE(ui_img_github);
LV_FONT_DECLARE(ui_font_portal_title_22);
#if !XTOUCH_UI_HIDPI_ASSETS
LV_FONT_DECLARE(ui_font_project_16);
#endif
static bool backlightDirty,screenTimeoutDirty;
#if XTOUCH_FEATURE_AUDIO
static bool volumeDirty;
static lv_obj_t *volumeValue;
#endif

#if XTOUCH_FEATURE_AUDIO
static void sound_setting_event(lv_event_t *e){
 bool enabled=lv_obj_has_state(lv_event_get_target(e),LV_STATE_CHECKED);
 if((uintptr_t)lv_event_get_user_data(e))xTouchConfig.xTouchWarningSound=enabled;
 else xTouchConfig.xTouchTouchSound=enabled;
 lv_msg_send(XTOUCH_SETTINGS_AUDIO,NULL);lv_msg_send(XTOUCH_SETTINGS_SAVE,NULL);
}
static lv_obj_t *sound_setting(lv_obj_t *parent,const char *title,bool checked,unsigned kind){
 lv_obj_t *row=lv_obj_create(parent);lv_obj_remove_style_all(row);
 lv_obj_set_size(row,LV_PCT(100),40);lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);
 lv_obj_set_flex_align(row,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
 lv_obj_set_style_pad_column(row,6,0);
 lv_obj_clear_flag(row,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_bg_color(row,lv_color_hex(0x222222),0);lv_obj_set_style_bg_opa(row,LV_OPA_COVER,0);lv_obj_set_style_radius(row,6,0);lv_obj_set_style_pad_left(row,16,0);lv_obj_set_style_pad_right(row,16,0);lv_obj_set_style_pad_ver(row,8,0);
 lv_obj_t *label=lv_label_create(row);lv_label_set_text(label,title);lv_obj_set_flex_grow(label,1);
 lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);ui_text_guard(label);lv_obj_add_event_cb(label,ui_layout_single_line_event,LV_EVENT_SIZE_CHANGED,NULL);
 lv_obj_t *toggle=lv_switch_create(row);lv_obj_set_size(toggle,46,24);
 lv_obj_set_style_bg_color(toggle,lv_color_hex(0x2AFF00),LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_set_style_bg_color(toggle,lv_color_hex(0x2AFF00),LV_PART_KNOB);lv_obj_set_style_bg_color(toggle,lv_color_black(),LV_PART_KNOB|LV_STATE_CHECKED);
 if(checked)lv_obj_add_state(toggle,LV_STATE_CHECKED);
 lv_obj_add_event_cb(toggle,sound_setting_event,LV_EVENT_VALUE_CHANGED,(void*)(uintptr_t)kind);
 return row;
}
static void sound_volume_event(lv_event_t *e){
 if(lv_event_get_code(e)==LV_EVENT_VALUE_CHANGED){
  xTouchConfig.xTouchSoundVolume=lv_slider_get_value(lv_event_get_target(e));volumeDirty=true;
  lv_label_set_text_fmt(volumeValue,"%u%%",(unsigned)xTouchConfig.xTouchSoundVolume);lv_msg_send(XTOUCH_SETTINGS_AUDIO,NULL);
 }else if(lv_event_get_code(e)==LV_EVENT_RELEASED&&volumeDirty){volumeDirty=false;lv_msg_send(XTOUCH_SETTINGS_SAVE,NULL);}
}
static lv_obj_t *sound_volume_setting(lv_obj_t *parent){
 lv_obj_t *row=lv_obj_create(parent);lv_obj_remove_style_all(row);lv_obj_set_size(row,LV_PCT(100),70);lv_obj_clear_flag(row,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_style_bg_color(row,lv_color_hex(0x222222),0);lv_obj_set_style_bg_opa(row,LV_OPA_COVER,0);lv_obj_set_style_radius(row,6,0);lv_obj_set_style_pad_left(row,16,0);lv_obj_set_style_pad_right(row,16,0);lv_obj_set_style_pad_ver(row,8,0);lv_obj_set_style_pad_row(row,6,0);lv_obj_set_flex_flow(row,LV_FLEX_FLOW_COLUMN);
 lv_obj_t *line=lv_obj_create(row);lv_obj_remove_style_all(line);lv_obj_clear_flag(line,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_size(line,LV_PCT(100),LV_SIZE_CONTENT);lv_obj_set_flex_flow(line,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(line,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
 lv_obj_t *title=lv_label_create(line);lv_label_set_text(title,xtouch_local("音量","Volume"));ui_text_guard(title);
 volumeValue=lv_label_create(line);lv_label_set_text_fmt(volumeValue,"%u%%",(unsigned)xTouchConfig.xTouchSoundVolume);lv_obj_set_style_text_color(volumeValue,lv_color_hex(0xFFFFFF),0);ui_text_guard(volumeValue);
 lv_obj_t *slider=lv_slider_create(row);lv_obj_set_size(slider,LV_PCT(100),10);lv_slider_set_range(slider,0,100);lv_slider_set_value(slider,xTouchConfig.xTouchSoundVolume,LV_ANIM_OFF);lv_obj_set_style_bg_color(slider,lv_color_hex(0x333333),LV_PART_MAIN);lv_obj_set_style_bg_color(slider,lv_color_hex(0x00FF00),LV_PART_INDICATOR);lv_obj_set_style_bg_color(slider,lv_color_hex(0xFFFFFF),LV_PART_KNOB);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB);lv_obj_add_event_cb(slider,sound_volume_event,LV_EVENT_ALL,NULL);volumeDirty=false;return row;
}

#endif

void ui_event_comp_settingsComponent_resetDeviceButton(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        onSettingsResetDevice(e);
    }
}

void ui_event_comp_settingsComponent_onBackLight(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        backlightDirty=true;
        lv_msg_send(XTOUCH_SETTINGS_BACKLIGHT, NULL);
    }
    if (event_code == LV_EVENT_RELEASED)
    {
        if(backlightDirty){backlightDirty=false;lv_msg_send(XTOUCH_SETTINGS_BACKLIGHT_SET, NULL);}
    }
}

void ui_event_comp_settingsComponent_tftInvertInput(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_msg_send(XTOUCH_SETTINGS_TFT_INVERT, NULL);
    }
}

void ui_event_comp_settingsComponent_onTFTOFF(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        screenTimeoutDirty=true;
        uint32_t value = lv_slider_get_value(ui_settingsTFTOFFSlider);
        lv_label_set_text_fmt(ui_settingsTFTOFFValue, "%dm", (int)value);
        if (value < XTOUCH_LCD_MIN_SLEEP_TIME)
        {
            lv_label_set_text(ui_settingsTFTOFFValue, XTOUCH_TEXT_OFF);
        }
        else
        {
            lv_label_set_text_fmt(ui_settingsTFTOFFValue, "%dm", (int)value);
        }
    }
    if (event_code == LV_EVENT_RELEASED)
    {
        if(screenTimeoutDirty){screenTimeoutDirty=false;lv_msg_send(XTOUCH_SETTINGS_TFTOFF_SET, NULL);}
    }
}

void ui_event_comp_settingsComponent_onWOP(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onSettingsWOP(e);
    }
}

void ui_event_comp_settingsComponent_onTFTFlip(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onSettingsTFTFlip(e);
    }
}

void ui_event_comp_settingsComponent_onOTA(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onSettingsOTA(e);
    }
}

void ui_event_comp_settingsComponent_onAuxFan(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onSettingsAuxFan(e);
    }
}

void ui_event_comp_settingsComponent_onChamberFan(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onSettingsChamberFan(e);
    }
}

void ui_event_comp_settingsComponent_onChamberTemp(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onSettingsChamberSensor(e);
    }
}

static lv_obj_t *settings_heading(lv_obj_t *parent,const char *text){
 lv_obj_t *title=lv_label_create(parent);lv_obj_set_size(title,LV_PCT(100),LV_SIZE_CONTENT);lv_label_set_text(title,text);lv_obj_set_style_bg_color(title,lv_color_hex(0x333333),0);lv_obj_set_style_bg_opa(title,LV_OPA_COVER,0);lv_obj_set_style_radius(title,6,0);lv_obj_set_style_pad_left(title,16,0);lv_obj_set_style_pad_right(title,16,0);lv_obj_set_style_pad_ver(title,12,0);return title;
}

// COMPONENT settingsComponent

static void onLanguageChanged(lv_event_t *e)
{
    xtouch_language_request(lv_dropdown_get_selected(lv_event_get_target(e)));
}

lv_obj_t *ui_settingsComponent_create(lv_obj_t *comp_parent)
{
    backlightDirty=screenTimeoutDirty=false;

    lv_obj_t *cui_settingsComponent;
    cui_settingsComponent = lv_obj_create(comp_parent);
    lv_obj_set_height(cui_settingsComponent, lv_pct(100));
    lv_obj_set_flex_grow(cui_settingsComponent, 1);
    lv_obj_set_x(cui_settingsComponent, 385);
    lv_obj_set_y(cui_settingsComponent, 178);
    lv_obj_set_flex_flow(cui_settingsComponent, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(cui_settingsComponent,LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scroll_dir(cui_settingsComponent,LV_DIR_VER);
    lv_obj_set_flex_align(cui_settingsComponent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_radius(cui_settingsComponent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_settingsComponent, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsComponent, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settingsComponent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsComponent, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsComponent, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsComponent, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsComponent, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_settingsComponent, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_settingsComponent, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_settingsComponent, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_settingsComponent, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(cui_settingsComponent, LV_SCROLLBAR_MODE_ACTIVE);

    lv_obj_set_style_bg_color(cui_settingsComponent, lv_color_hex(0x2AFF00), LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsComponent, 255, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    lv_obj_t *cui_settingsTitle;
    cui_settingsTitle = lv_label_create(cui_settingsComponent);
    lv_obj_set_width(cui_settingsTitle, lv_pct(100));
    lv_obj_set_height(cui_settingsTitle, LV_SIZE_CONTENT); /// 40
    lv_label_set_text(cui_settingsTitle, XTOUCH_TEXT_SETTINGS);
    lv_obj_set_scrollbar_mode(cui_settingsTitle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_settingsTitle, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsTitle, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsTitle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsTitle, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsTitle, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_settingsTitle, lv_color_hex(0xFF682A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsTitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settingsTitle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_settingsTitle, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_settingsTitle, lv_color_hex(0x000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *languageRow = lv_obj_create(cui_settingsComponent);
    lv_obj_remove_style_all(languageRow);
    lv_obj_set_size(languageRow, lv_pct(100), 48);
    lv_obj_set_style_bg_color(languageRow,lv_color_hex(0x222222),0);
    lv_obj_set_style_bg_opa(languageRow,LV_OPA_COVER,0);
    lv_obj_set_style_radius(languageRow,6,0);
    lv_obj_set_style_pad_left(languageRow,16,0);
    lv_obj_set_style_pad_right(languageRow,16,0);
    lv_obj_set_flex_flow(languageRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(languageRow, LV_FLEX_ALIGN_SPACE_BETWEEN,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *languageLabel = lv_label_create(languageRow);
    lv_label_set_text(languageLabel, XTOUCH_TEXT_LANGUAGE);
    lv_obj_t *languageSelect = lv_dropdown_create(languageRow);
    lv_obj_set_size(languageSelect,132,34);
    lv_obj_set_style_bg_color(languageSelect,lv_color_hex(0x333333),0);
    lv_obj_set_style_text_color(languageSelect,lv_color_white(),0);
    lv_obj_set_style_border_width(languageSelect,0,0);
    lv_obj_set_style_pad_ver(languageSelect,6,0);
    lv_obj_t *languageList=lv_dropdown_get_list(languageSelect);
    lv_obj_set_style_bg_color(languageList,lv_color_hex(0x333333),0);
    lv_obj_set_style_text_color(languageList,lv_color_white(),0);
    lv_obj_set_style_bg_color(languageList,lv_color_hex(0x555555),LV_PART_SELECTED|LV_STATE_CHECKED);
    lv_dropdown_set_options(languageSelect, "简体中文\n繁體中文\nEnglish\nFrançais\nDeutsch\nРусский\n日本語\n한국어");
    lv_dropdown_set_selected(languageSelect, xtouch_language_get());
    ui_layout_dropdown(languageSelect);
    lv_obj_add_event_cb(languageSelect, onLanguageChanged, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *cui_screenTitle;
    cui_screenTitle = lv_label_create(cui_settingsComponent);
    lv_obj_set_width(cui_screenTitle, lv_pct(100));
    lv_obj_set_height(cui_screenTitle, LV_SIZE_CONTENT); /// 40
    lv_label_set_text(cui_screenTitle, XTOUCH_TEXT_DISPLAY);
    lv_obj_set_scrollbar_mode(cui_screenTitle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_screenTitle, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_screenTitle, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_screenTitle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_screenTitle, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_screenTitle, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_screenTitle, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_screenTitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_screenTitle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_screenTitle, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_screenTitle, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settingsBackLightPanel;
    cui_settingsBackLightPanel = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settingsBackLightPanel, lv_pct(100));
    lv_obj_set_height(cui_settingsBackLightPanel, 70);
    lv_obj_set_flex_flow(cui_settingsBackLightPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settingsBackLightPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(cui_settingsBackLightPanel, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_settingsBackLightPanel, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsBackLightPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settingsBackLightPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsBackLightPanel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsBackLightPanel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsBackLightPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsBackLightPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settingsBackLightPanelLabel;
    cui_settingsBackLightPanelLabel = lv_label_create(cui_settingsBackLightPanel);
    lv_obj_set_width(cui_settingsBackLightPanelLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settingsBackLightPanelLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settingsBackLightPanelLabel, XTOUCH_TEXT_BACK);
    lv_obj_set_style_text_font(cui_settingsBackLightPanelLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsBackLightPanelLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsBackLightPanelLabel, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsBackLightPanelLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsBackLightPanelLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // lv_obj_t *ui_settingsBackLightPanelSlider;
    ui_settingsBackLightPanelSlider = lv_slider_create(cui_settingsBackLightPanel);
    lv_slider_set_range(ui_settingsBackLightPanelSlider, 10, 255);
    lv_slider_set_value(ui_settingsBackLightPanelSlider, xTouchConfig.xTouchBacklightLevel, LV_ANIM_OFF);
    lv_obj_set_height(ui_settingsBackLightPanelSlider, 10);
    lv_obj_set_flex_grow(ui_settingsBackLightPanelSlider, 1);
    lv_obj_set_x(ui_settingsBackLightPanelSlider, 9);
    lv_obj_set_y(ui_settingsBackLightPanelSlider, 28);
    lv_obj_set_style_bg_color(ui_settingsBackLightPanelSlider, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsBackLightPanelSlider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_settingsBackLightPanelSlider, lv_color_hex(0x00FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsBackLightPanelSlider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_settingsBackLightPanelSlider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsBackLightPanelSlider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_t *cui_settingsTFTOFFPanel;
    cui_settingsTFTOFFPanel = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settingsTFTOFFPanel, lv_pct(100));
    lv_obj_set_height(cui_settingsTFTOFFPanel, 70);
    lv_obj_set_flex_flow(cui_settingsTFTOFFPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settingsTFTOFFPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(cui_settingsTFTOFFPanel, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_settingsTFTOFFPanel, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsTFTOFFPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settingsTFTOFFPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsTFTOFFPanel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsTFTOFFPanel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsTFTOFFPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsTFTOFFPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settingsTFTOFFPanelLabel;
    cui_settingsTFTOFFPanelLabel = lv_label_create(cui_settingsTFTOFFPanel);
    lv_obj_set_width(cui_settingsTFTOFFPanelLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settingsTFTOFFPanelLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settingsTFTOFFPanelLabel, XTOUCH_TEXT_SLEEP);
    lv_obj_set_style_text_font(cui_settingsTFTOFFPanelLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsTFTOFFPanelLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsTFTOFFPanelLabel, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsTFTOFFPanelLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsTFTOFFPanelLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_settingsTFTOFFSlider = lv_slider_create(cui_settingsTFTOFFPanel);
    lv_slider_set_range(ui_settingsTFTOFFSlider, 4, 60);
    lv_slider_set_value(ui_settingsTFTOFFSlider, xTouchConfig.xTouchTFTOFFValue, LV_ANIM_OFF);
    lv_obj_set_height(ui_settingsTFTOFFSlider, 10);
    lv_obj_set_flex_grow(ui_settingsTFTOFFSlider, 1);
    lv_obj_set_x(ui_settingsTFTOFFSlider, 9);
    lv_obj_set_y(ui_settingsTFTOFFSlider, 28);
    lv_obj_set_style_bg_color(ui_settingsTFTOFFSlider, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsTFTOFFSlider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_settingsTFTOFFSlider, lv_color_hex(0x00FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsTFTOFFSlider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_settingsTFTOFFSlider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsTFTOFFSlider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_settingsTFTOFFValue = lv_label_create(cui_settingsTFTOFFPanel);
    lv_obj_set_width(ui_settingsTFTOFFValue, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_settingsTFTOFFValue, LV_SIZE_CONTENT); /// 1

    int32_t value = lv_slider_get_value(ui_settingsTFTOFFSlider);
    if (value < XTOUCH_LCD_MIN_SLEEP_TIME)
        lv_label_set_text(ui_settingsTFTOFFValue, XTOUCH_TEXT_OFF);
    else
        lv_label_set_text_fmt(ui_settingsTFTOFFValue, "%dm", (int)value);
    lv_obj_set_style_text_font(ui_settingsTFTOFFValue, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_settingsTFTOFFValue, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_settingsTFTOFFValue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_settingsTFTOFFValue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_settingsTFTOFFValue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_screenWOPSetting;
    cui_screenWOPSetting = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_screenWOPSetting, lv_pct(100));
    lv_obj_set_height(cui_screenWOPSetting, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_screenWOPSetting, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_screenWOPSetting, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_screenWOPSetting, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_screenWOPSetting, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_screenWOPSetting, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_screenWOPSetting, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_screenWOPSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_screenWOPSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_screenWOPSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_screenWOPSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_screenWOPLabel;
    cui_screenWOPLabel = lv_label_create(cui_screenWOPSetting);
    lv_obj_set_width(cui_screenWOPLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_screenWOPLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_screenWOPLabel, XTOUCH_TEXT_WAKE_ON_PRINT);
    lv_obj_set_scrollbar_mode(cui_screenWOPLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settingsWOPSwitch = lv_switch_create(cui_screenWOPSetting);
    lv_obj_set_width(ui_settingsWOPSwitch, 50);
    lv_obj_set_height(ui_settingsWOPSwitch, 25);

    lv_obj_set_style_bg_color(ui_settingsWOPSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settingsWOPSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settingsWOPSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsWOPSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settingsWOPSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settingsWOPSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);

    if (xTouchConfig.xTouchWakeOnPrint)
    {
        lv_obj_add_state(ui_settingsWOPSwitch, LV_STATE_CHECKED);
    }

    lv_obj_t *cui_screenInvertSetting;
    cui_screenInvertSetting = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_screenInvertSetting, lv_pct(100));
    lv_obj_set_height(cui_screenInvertSetting, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_screenInvertSetting, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_screenInvertSetting, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_screenInvertSetting, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_screenInvertSetting, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_screenInvertSetting, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_screenInvertSetting, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_screenInvertSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_screenInvertSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_screenInvertSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_screenInvertSetting, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_screenInvertLabel;
    cui_screenInvertLabel = lv_label_create(cui_screenInvertSetting);
    lv_obj_set_width(cui_screenInvertLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_screenInvertLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_screenInvertLabel, XTOUCH_TEXT_INVERT_COLORS);
    lv_obj_set_scrollbar_mode(cui_screenInvertLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settingsTFTInvertSwitch = lv_switch_create(cui_screenInvertSetting);
    lv_obj_set_width(ui_settingsTFTInvertSwitch, 50);
    lv_obj_set_height(ui_settingsTFTInvertSwitch, 25);

    lv_obj_set_style_bg_color(ui_settingsTFTInvertSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settingsTFTInvertSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settingsTFTInvertSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsTFTInvertSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settingsTFTInvertSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settingsTFTInvertSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);
    if (xTouchConfig.xTouchTFTInvert)
    {
        lv_obj_add_state(ui_settingsTFTInvertSwitch, LV_STATE_CHECKED);
    }

    lv_obj_t *cui_settingsTFTFlip;
    cui_settingsTFTFlip = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settingsTFTFlip, lv_pct(100));
    lv_obj_set_height(cui_settingsTFTFlip, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_settingsTFTFlip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settingsTFTFlip, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_settingsTFTFlip, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_settingsTFTFlip, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settingsTFTFlip, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settingsTFTFlip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settingsTFTFlip, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settingsTFTFlip, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settingsTFTFlip, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settingsTFTFlip, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settingsTFTFlipLabel;
    cui_settingsTFTFlipLabel = lv_label_create(cui_settingsTFTFlip);
    lv_obj_set_width(cui_settingsTFTFlipLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settingsTFTFlipLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settingsTFTFlipLabel, XTOUCH_TEXT_FLIP_SCREEN);
    lv_obj_set_scrollbar_mode(cui_settingsTFTFlipLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settingsTFTFlipSwitch = lv_switch_create(cui_settingsTFTFlip);
    lv_obj_set_width(ui_settingsTFTFlipSwitch, 50);
    lv_obj_set_height(ui_settingsTFTFlipSwitch, 25);

    lv_obj_set_style_bg_color(ui_settingsTFTFlipSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settingsTFTFlipSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settingsTFTFlipSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settingsTFTFlipSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settingsTFTFlipSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settingsTFTFlipSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);
    if (xTouchConfig.xTouchTFTFlip)
    {
        lv_obj_add_state(ui_settingsTFTFlipSwitch, LV_STATE_CHECKED);
    }

    lv_obj_t *cui_deviceTitle;
    cui_deviceTitle = lv_label_create(cui_settingsComponent);
    lv_obj_set_width(cui_deviceTitle, lv_pct(100));
    lv_obj_set_height(cui_deviceTitle, LV_SIZE_CONTENT); /// 40
    lv_label_set_text_fmt(cui_deviceTitle, "XTOUCH-CN v%s", XTOUCH_FIRMWARE_VERSION);
    lv_obj_add_flag(cui_deviceTitle,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cui_deviceTitle,xtouch_ota_version_tap,LV_EVENT_ALL,NULL);
    lv_obj_set_scrollbar_mode(cui_deviceTitle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_deviceTitle, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_deviceTitle, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_deviceTitle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_deviceTitle, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_deviceTitle, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_deviceTitle, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_deviceTitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_deviceTitle, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_deviceTitle, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_deviceTitle, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settings_auxFan;
    cui_settings_auxFan = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settings_auxFan, lv_pct(100));
    lv_obj_set_height(cui_settings_auxFan, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_settings_auxFan, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settings_auxFan, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_settings_auxFan, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_settings_auxFan, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settings_auxFan, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settings_auxFan, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settings_auxFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settings_auxFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settings_auxFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settings_auxFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settings_auxFanLabel;
    cui_settings_auxFanLabel = lv_label_create(cui_settings_auxFan);
    lv_obj_set_width(cui_settings_auxFanLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settings_auxFanLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settings_auxFanLabel, XTOUCH_TEXT_AUX_FAN);
    lv_obj_set_scrollbar_mode(cui_settings_auxFanLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settings_auxFanSwitch = lv_switch_create(cui_settings_auxFan);
    lv_obj_set_width(ui_settings_auxFanSwitch, 50);
    lv_obj_set_height(ui_settings_auxFanSwitch, 25);

    lv_obj_set_style_bg_color(ui_settings_auxFanSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_auxFanSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settings_auxFanSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settings_auxFanSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settings_auxFanSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_auxFanSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);

    if (!xtouch_bblp_is_p1p() && !xtouch_bblp_is_x1())
    {
        lv_obj_add_flag(cui_settings_auxFan, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (xTouchConfig.xTouchAuxFanEnabled)
        {
            lv_obj_add_state(ui_settings_auxFanSwitch, LV_STATE_CHECKED);
        }
    }

    lv_obj_t *cui_settings_chamberFan;
    cui_settings_chamberFan = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settings_chamberFan, lv_pct(100));
    lv_obj_set_height(cui_settings_chamberFan, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_settings_chamberFan, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settings_chamberFan, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_settings_chamberFan, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_settings_chamberFan, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settings_chamberFan, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settings_chamberFan, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settings_chamberFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settings_chamberFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settings_chamberFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settings_chamberFan, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settings_chamberFanLabel;
    cui_settings_chamberFanLabel = lv_label_create(cui_settings_chamberFan);
    lv_obj_set_width(cui_settings_chamberFanLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settings_chamberFanLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settings_chamberFanLabel, XTOUCH_TEXT_CHAMBER_FAN);
    lv_obj_set_scrollbar_mode(cui_settings_chamberFanLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settings_chamberFanSwitch = lv_switch_create(cui_settings_chamberFan);
    lv_obj_set_width(ui_settings_chamberFanSwitch, 50);
    lv_obj_set_height(ui_settings_chamberFanSwitch, 25);

    lv_obj_set_style_bg_color(ui_settings_chamberFanSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_chamberFanSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settings_chamberFanSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settings_chamberFanSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settings_chamberFanSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_chamberFanSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);

    if (!xtouch_bblp_is_p1p())
    {
        lv_obj_add_flag(cui_settings_chamberFan, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (xTouchConfig.xTouchChamberFanEnabled)
        {
            lv_obj_add_state(ui_settings_chamberFanSwitch, LV_STATE_CHECKED);
        }
    }

    lv_obj_t *cui_settings_chamberSensor;
    cui_settings_chamberSensor = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settings_chamberSensor, lv_pct(100));
    lv_obj_set_height(cui_settings_chamberSensor, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_settings_chamberSensor, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settings_chamberSensor, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_settings_chamberSensor, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_settings_chamberSensor, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settings_chamberSensor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settings_chamberSensor, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settings_chamberSensor, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settings_chamberSensor, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settings_chamberSensor, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settings_chamberSensor, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settings_chamberSensorLabel;
    cui_settings_chamberSensorLabel = lv_label_create(cui_settings_chamberSensor);
    lv_obj_set_width(cui_settings_chamberSensorLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settings_chamberSensorLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settings_chamberSensorLabel, XTOUCH_TEXT_CHAMBER_TEMP);
    lv_obj_set_scrollbar_mode(cui_settings_chamberSensorLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settings_chamberSensorSwitch = lv_switch_create(cui_settings_chamberSensor);
    lv_obj_set_width(ui_settings_chamberSensorSwitch, 50);
    lv_obj_set_height(ui_settings_chamberSensorSwitch, 25);

    lv_obj_set_style_bg_color(ui_settings_chamberSensorSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_chamberSensorSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settings_chamberSensorSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settings_chamberSensorSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settings_chamberSensorSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_chamberSensorSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);

    if (!xtouch_bblp_is_p1Series())
    {
        lv_obj_add_flag(cui_settings_chamberSensor, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (xTouchConfig.xTouchChamberSensorEnabled)
        {
            lv_obj_add_state(ui_settings_chamberSensorSwitch, LV_STATE_CHECKED);
        }
    }

    lv_obj_t *cui_settings_ota;
    cui_settings_ota = lv_obj_create(cui_settingsComponent);
    lv_obj_set_width(cui_settings_ota, lv_pct(100));
    lv_obj_set_height(cui_settings_ota, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_settings_ota, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_settings_ota, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cui_settings_ota, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_settings_ota, lv_color_hex(0x222222), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_settings_ota, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_settings_ota, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_settings_ota, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_settings_ota, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_settings_ota, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_settings_ota, 16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_settings_otaLabel;
    cui_settings_otaLabel = lv_label_create(cui_settings_ota);
    lv_obj_set_width(cui_settings_otaLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_settings_otaLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_settings_otaLabel, XTOUCH_TEXT_OTA_UPDATE);
    lv_obj_set_scrollbar_mode(cui_settings_otaLabel, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *ui_settingsTFTInvertSwitch;
    ui_settings_otaSwitch = lv_switch_create(cui_settings_ota);
    lv_obj_set_width(ui_settings_otaSwitch, 50);
    lv_obj_set_height(ui_settings_otaSwitch, 25);

    lv_obj_set_style_bg_color(ui_settings_otaSwitch, lv_color_hex(0x2AFF00), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_otaSwitch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ui_settings_otaSwitch, lv_color_hex(0x2AFF00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_settings_otaSwitch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_settings_otaSwitch, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_settings_otaSwitch, 255, LV_PART_KNOB | LV_STATE_CHECKED);

    if (xTouchConfig.xTouchOTAEnabled)
    {
        lv_obj_add_state(ui_settings_otaSwitch, LV_STATE_CHECKED);
    }

#if XTOUCH_FEATURE_AUDIO
    lv_obj_t *soundTitle=settings_heading(cui_settingsComponent,xtouch_local("声音","Sound"));
    lv_obj_t *touchSoundRow=sound_setting(cui_settingsComponent,xtouch_local("触摸提示音","Touch sound"),xTouchConfig.xTouchTouchSound,0);
    lv_obj_t *warningSoundRow=sound_setting(cui_settingsComponent,xtouch_local("警告提示音","Warning sound"),xTouchConfig.xTouchWarningSound,1);
    lv_obj_t *volumeRow=sound_volume_setting(cui_settingsComponent);
#endif
    lv_obj_t *cui_reseDeviceButton;
    cui_reseDeviceButton = lv_label_create(cui_settingsComponent);
    lv_obj_set_width(cui_reseDeviceButton, lv_pct(100));
    lv_obj_set_height(cui_reseDeviceButton, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_reseDeviceButton, XTOUCH_TEXT_REBOOT_DEVICE);
    lv_obj_add_flag(cui_reseDeviceButton, LV_OBJ_FLAG_CLICKABLE);    /// Flags
    lv_obj_clear_flag(cui_reseDeviceButton, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_scrollbar_mode(cui_reseDeviceButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_reseDeviceButton, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_reseDeviceButton, lv_color_hex(0xFF682A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_reseDeviceButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_reseDeviceButton, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_reseDeviceButton, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_reseDeviceButton, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_reseDeviceButton, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_reseDeviceButton, lv_color_hex(0x000), LV_PART_MAIN | LV_STATE_DEFAULT);

    // This device has no browser: show the complete project URL as a single line.
    lv_obj_t *projectRow = lv_obj_create(cui_settingsComponent);
    lv_obj_remove_style_all(projectRow);
    lv_obj_set_size(projectRow, lv_pct(100), 40);
    lv_obj_set_flex_flow(projectRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(projectRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(projectRow, 5, 0);
    lv_obj_clear_flag(projectRow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(projectRow,lv_color_hex(0x222222),0);lv_obj_set_style_bg_opa(projectRow,LV_OPA_COVER,0);
    lv_obj_set_style_radius(projectRow,8,0);lv_obj_set_style_pad_left(projectRow,16,0);lv_obj_set_style_pad_right(projectRow,16,0);lv_obj_set_style_pad_ver(projectRow,8,0);

    lv_obj_t *projectIcon = lv_img_create(projectRow);
    lv_img_set_src(projectIcon, &ui_img_github);
    lv_obj_set_style_img_recolor(projectIcon,lv_color_white(),0);lv_obj_set_style_img_recolor_opa(projectIcon,LV_OPA_COVER,0);
    lv_obj_t *projectUrl = lv_label_create(projectRow);
    #if XTOUCH_UI_HIDPI_ASSETS
    lv_obj_set_style_text_font(projectUrl, &ui_font_portal_title_22, 0);
#else
    lv_obj_set_style_text_font(projectUrl, &ui_font_project_16, 0);
#endif
    lv_obj_set_style_text_color(projectUrl, lv_color_hex(UI_COLOR_BLUE), 0);
    lv_obj_set_style_text_letter_space(projectUrl, 2, 0);
    lv_obj_set_flex_grow(projectUrl,0);lv_obj_set_width(projectUrl,LV_SIZE_CONTENT);lv_obj_set_style_text_align(projectUrl,LV_TEXT_ALIGN_LEFT,0);
    lv_label_set_long_mode(projectUrl, LV_LABEL_LONG_CLIP);
    lv_label_set_text(projectUrl, lv_disp_get_hor_res(NULL)<=480 ? "tbc0309/xTouch-CN" : "github.com/tbc0309/xTouch-CN");
    lv_obj_add_event_cb(projectRow,ui_layout_project_event,LV_EVENT_SIZE_CHANGED,NULL);
    lv_obj_update_layout(projectRow);lv_event_send(projectRow,LV_EVENT_SIZE_CHANGED,NULL);

    lv_obj_t *manageButton=lv_btn_create(cui_settingsComponent);
    lv_obj_set_size(manageButton,LV_PCT(100),40);ui_layout_setup_button(manageButton,true);
    ui_layout_setup_caption(manageButton,xtouch_local("网页管理 / 扫码", "Web management / QR"));
    lv_obj_add_event_cb(manageButton,portal_manage_event,LV_EVENT_CLICKED,NULL);
    lv_obj_t *wifiButton=lv_btn_create(cui_settingsComponent);
    lv_obj_set_size(wifiButton,LV_PCT(100),40);ui_layout_setup_button(wifiButton,false);
    ui_layout_setup_caption(wifiButton,xtouch_network_setup_label(false));
    lv_obj_add_event_cb(wifiButton,portal_wifi_event,LV_EVENT_CLICKED,NULL);

#if XTOUCH_FEATURE_BLE_PROVISIONING
    lv_obj_t *bleButton=lv_btn_create(cui_settingsComponent);
    lv_obj_set_size(bleButton,LV_PCT(100),40);ui_layout_setup_button(bleButton,false);
    ui_layout_setup_caption(bleButton,xtouch_network_setup_label(true));
    lv_obj_add_event_cb(bleButton,portal_ble_event,LV_EVENT_CLICKED,NULL);
#endif
    lv_obj_t *standbyTitle=settings_heading(cui_settingsComponent,xtouch_local("待机","Standby"));
    lv_obj_t *printerTitle=settings_heading(cui_settingsComponent,xtouch_local("打印机","Printer"));
    if(lv_obj_has_flag(cui_settings_auxFan,LV_OBJ_FLAG_HIDDEN)&&lv_obj_has_flag(cui_settings_chamberFan,LV_OBJ_FLAG_HIDDEN)&&lv_obj_has_flag(cui_settings_chamberSensor,LV_OBJ_FLAG_HIDDEN))lv_obj_add_flag(printerTitle,LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ordered[]={cui_settingsTitle,cui_screenTitle,languageRow,cui_settingsBackLightPanel,cui_screenInvertSetting,cui_settingsTFTFlip,standbyTitle,cui_settingsTFTOFFPanel,cui_screenWOPSetting,
#if XTOUCH_FEATURE_AUDIO
    soundTitle,touchSoundRow,warningSoundRow,volumeRow,
#endif
    printerTitle,cui_settings_auxFan,cui_settings_chamberFan,cui_settings_chamberSensor,cui_deviceTitle,cui_settings_ota,cui_reseDeviceButton,projectRow,manageButton,wifiButton
#if XTOUCH_FEATURE_BLE_PROVISIONING
    ,bleButton
#endif
    };
    for(unsigned n=0;n<sizeof(ordered)/sizeof(ordered[0]);++n){lv_obj_move_to_index(ordered[n],n);lv_obj_clear_flag(ordered[n],LV_OBJ_FLAG_SCROLLABLE);}

    lv_obj_t **children = lv_mem_alloc(sizeof(lv_obj_t *) * _UI_COMP_SETTINGSCOMPONENT_NUM);
    children[UI_COMP_SETTINGSCOMPONENT_SETTINGSCOMPONENT] = cui_settingsComponent;
    children[UI_COMP_SETTINGSCOMPONENT_SCREEN_TITLE] = cui_screenTitle;
    children[UI_COMP_SETTINGSCOMPONENT_BACKLIGHT] = cui_settingsBackLightPanel;
    children[UI_COMP_SETTINGSCOMPONENT_BACKLIGHT_LABEL] = cui_settingsBackLightPanelLabel;
    children[UI_COMP_SETTINGSCOMPONENT_BACKLIGHT_SLIDER] = ui_settingsBackLightPanelSlider;
    children[UI_COMP_SETTINGSCOMPONENT_TFTOFF] = cui_settingsTFTOFFPanel;
    children[UI_COMP_SETTINGSCOMPONENT_TFTOFF_LABEL] = cui_settingsTFTOFFPanelLabel;
    children[UI_COMP_SETTINGSCOMPONENT_TFTOFF_SLIDER] = ui_settingsTFTOFFSlider;
    children[UI_COMP_SETTINGSCOMPONENT_TFTOFF_VALUE] = ui_settingsTFTOFFValue;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_WOP] = cui_screenWOPSetting;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_WOP_LABEL] = cui_screenWOPLabel;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_WOP_INPUT] = ui_settingsWOPSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_INVERT] = cui_screenInvertSetting;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_INVERT_LABEL] = cui_screenInvertLabel;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_INVERT_INPUT] = ui_settingsTFTInvertSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_FLIP] = cui_settingsTFTFlip;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_FLIP_LABEL] = cui_settingsTFTFlipLabel;
    children[UI_COMP_SETTINGSCOMPONENT_TFT_FLIP_INPUT] = ui_settingsTFTFlipSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_DEVICE_TITLE] = cui_deviceTitle;
    children[UI_COMP_SETTINGSCOMPONENT_AUXFAN] = cui_settings_auxFan;
    children[UI_COMP_SETTINGSCOMPONENT_AUXFAN_LABEL] = cui_settings_auxFanLabel;
    children[UI_COMP_SETTINGSCOMPONENT_AUXFAN_SWITCH] = ui_settings_auxFanSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_CHAMBERFAN] = cui_settings_chamberFan;
    children[UI_COMP_SETTINGSCOMPONENT_CHAMBERFAN_LABEL] = cui_settings_chamberFanLabel;
    children[UI_COMP_SETTINGSCOMPONENT_CHAMBERFAN_SWITCH] = ui_settings_chamberFanSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_CHAMBER_TEMP] = cui_settings_chamberSensor;
    children[UI_COMP_SETTINGSCOMPONENT_CHAMBER_TEMP_LABEL] = cui_settings_chamberSensorLabel;
    children[UI_COMP_SETTINGSCOMPONENT_CHAMBER_TEMP_SWITCH] = ui_settings_chamberSensorSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_OTA] = cui_settings_ota;
    children[UI_COMP_SETTINGSCOMPONENT_OTA_LABEL] = cui_settings_otaLabel;
    children[UI_COMP_SETTINGSCOMPONENT_OTA_SWITCH] = ui_settings_otaSwitch;
    children[UI_COMP_SETTINGSCOMPONENT_RESETDEVICEBUTTON] = cui_reseDeviceButton;

    lv_obj_add_event_cb(cui_settingsComponent, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children);
    lv_obj_add_event_cb(cui_settingsComponent, del_component_child_event_cb, LV_EVENT_DELETE, children);

    lv_obj_add_event_cb(cui_reseDeviceButton, ui_event_comp_settingsComponent_resetDeviceButton, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_settingsTFTOFFSlider, ui_event_comp_settingsComponent_onTFTOFF, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_settingsBackLightPanelSlider, ui_event_comp_settingsComponent_onBackLight, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_settingsWOPSwitch, ui_event_comp_settingsComponent_onWOP, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_settingsTFTInvertSwitch, ui_event_comp_settingsComponent_tftInvertInput, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(ui_settings_auxFanSwitch, ui_event_comp_settingsComponent_onAuxFan, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_settings_chamberFanSwitch, ui_event_comp_settingsComponent_onChamberFan, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_settings_chamberSensorSwitch, ui_event_comp_settingsComponent_onChamberTemp, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_settingsTFTFlipSwitch, ui_event_comp_settingsComponent_onTFTFlip, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_settings_otaSwitch, ui_event_comp_settingsComponent_onOTA, LV_EVENT_VALUE_CHANGED, NULL);

    ui_settings_gesture_bind(cui_settingsComponent);
    ui_comp_settingsComponent_create_hook(cui_settingsComponent);
    return cui_settingsComponent;
}
