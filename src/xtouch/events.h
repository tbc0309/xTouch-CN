#ifndef _XLCD_EVENTS
#define _XLCD_EVENTS

void xtouch_events_onResetDevice(lv_msg_t *m)
{
    if (xtouch_settings_flush()) ESP.restart();
}

void xtouch_events_onCloudSelect(lv_msg_t *m)
{
    // xtouch_cloud_pair_loop_exit = true;
}

void xtouch_events_onBackLight(lv_msg_t *m)
{
    int32_t value = lv_slider_get_value(ui_settingsBackLightPanelSlider);
    xTouchConfig.xTouchBacklightLevel = value;
    xtouch_screen_setBrightness(value);
}

void xtouch_events_onBackLightSet(lv_msg_t *m)
{
    int32_t value = lv_slider_get_value(ui_settingsBackLightPanelSlider);
    xTouchConfig.xTouchBacklightLevel = value;
    xtouch_settings_request_save();
    xtouch_screen_setBrightness(value);
}

void xtouch_events_onTFTTimerSet(lv_msg_t *m)
{
    int32_t value = lv_slider_get_value(ui_settingsTFTOFFSlider);
    xtouch_screen_setScreenTimer(value * 1000 * 60);

    xTouchConfig.xTouchTFTOFFValue = value;
    xtouch_settings_request_save();
}

void xtouch_events_onTFTInvert(lv_msg_t *m)
{
    bool value = lv_obj_has_state(ui_settingsTFTInvertSwitch, LV_STATE_CHECKED);
    xTouchConfig.xTouchTFTInvert = value;
    xtouch_settings_request_save();
    xtouch_screen_invertColors();
}

void xtouch_events_onSettingsSave(lv_msg_t *m)
{
    xtouch_settings_request_save();
}

void xtouch_events_onTFTFlip(lv_msg_t *m)
{
    xtouch_screen_toggleTFTFlip();
}

void xtouch_events_onResetTouch(lv_msg_t *m)
{
    xtouch_resetTouchConfig();
}

void xtouch_events_onChamberTempSwitch(lv_msg_t *m)
{
    if (xTouchConfig.xTouchChamberSensorEnabled)
    {
        xtouch_chamber_timer_start();
    }
    else
    {
        xtouch_chamber_timer_stop();
    }
}

void xtouch_events_onAudio(lv_msg_t *){xtouch_sound_begin();}

void xtouch_setupGlobalEvents()
{
    lv_msg_subscribe(XTOUCH_SETTINGS_AUDIO, xtouch_message_callback<xtouch_events_onAudio>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_RESET_DEVICE, xtouch_message_callback<xtouch_events_onResetDevice>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_BACKLIGHT, xtouch_message_callback<xtouch_events_onBackLight>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_BACKLIGHT_SET, xtouch_message_callback<xtouch_events_onBackLightSet>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_TFTOFF_SET, xtouch_message_callback<xtouch_events_onTFTTimerSet>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_TFT_INVERT, xtouch_message_callback<xtouch_events_onTFTInvert>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_SAVE, xtouch_message_callback<xtouch_events_onSettingsSave>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_CHAMBER_TEMP, xtouch_message_callback<xtouch_events_onChamberTempSwitch>, NULL);
    lv_msg_subscribe(XTOUCH_SETTINGS_TFT_FLIP, xtouch_message_callback<xtouch_events_onTFTFlip>, NULL);
}

#endif
