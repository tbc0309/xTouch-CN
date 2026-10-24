#ifndef _XLCD_SETTINGS
#define _XLCD_SETTINGS
#include "ui/settings_gesture.h"

// Read only the persisted display language before constructing the first screen.
// Full settings (and any SD restore) are applied later by the normal boot path.
void xtouch_settings_loadBootLanguage()
{
    xtouch_language_set_code("en");
    if (!xtouch_storage_has(xtouch_paths_settings)) return;
    auto settings = xtouch_storage_read(xtouch_paths_settings, 2048);
    xtouch_language_set_code(settings["language"] | "en");
}

bool xtouch_settings_save(bool syncSD = true)
{
    DynamicJsonDocument doc(1280);
    doc["timezone"] = xTouchConfig.xTouchTimezone;
    doc["timezoneName"] = xTouchConfig.xTouchTimezoneName;
    doc["ntpServer"] = xTouchConfig.xTouchNtpServer[0] ? xTouchConfig.xTouchNtpServer : "ntp.aliyun.com";
    doc["browserTimezoneSync"] = xTouchConfig.xTouchBrowserTimezoneSync;
    doc["language"] = xtouch_language_code(xtouch_language_get());
    doc["portalLanguage"] = xTouchConfig.xTouchPortalLanguage[0] ? xTouchConfig.xTouchPortalLanguage : "en";
    doc["backlight"] = xTouchConfig.xTouchBacklightLevel;
    doc["tftOff"] = xTouchConfig.xTouchTFTOFFValue;
    doc["tftInvert"] = xTouchConfig.xTouchTFTInvert;
    doc["printerPickerList"] = xTouchConfig.xTouchPrinterPickerList;
    doc["ota"] = xTouchConfig.xTouchOTAEnabled;
    doc["wop"] = xTouchConfig.xTouchWakeOnPrint;
    doc["soundVolume"]=xTouchConfig.xTouchSoundVolume;
    doc["touchSound"]=xTouchConfig.xTouchTouchSound;doc["warningSound"]=xTouchConfig.xTouchWarningSound;doc["statusLed"]=xTouchConfig.xTouchStatusLed;
    doc["deepSleepAfter"]=xTouchConfig.xTouchDeepSleepAfter;
    doc["chamberTempDiff"] = xTouchConfig.xTouchChamberSensorReadingDiff;
    doc["chamberTemp"] = xTouchConfig.xTouchChamberSensorEnabled;
    doc["auxFan"] = xTouchConfig.xTouchAuxFanEnabled;
    doc["chamberFan"] = xTouchConfig.xTouchChamberFanEnabled;

    const bool saved = xtouch_storage_write(xtouch_paths_settings, doc, syncSD);
    if (!saved) ConsoleError.println("[STORE] settings save failed");
    return saved;
}

// Touch handlers update RAM immediately. Persist only after the gesture and
// switch animation have finished; the normal idle SD poll mirrors dirty files.
static bool xtouch_settings_pending = false;
static uint32_t xtouch_settings_changed_at = 0;
void xtouch_settings_request_save() {
    xtouch_settings_pending = true;
    xtouch_settings_changed_at = millis();
}
bool xtouch_settings_flush() {
    if (!xtouch_settings_pending) return true;
    const uint32_t started = millis();
    if (!xtouch_settings_save(false)) {
        xtouch_settings_changed_at = millis();
        return false;
    }
    xtouch_settings_pending = false;
    Serial.printf("[SETTINGS] persisted in %u ms\n", unsigned(millis() - started));
    return true;
}
void xtouch_settings_poll() {
    if (!xtouch_settings_pending || millis() - xtouch_settings_changed_at < 500 ||
        lv_disp_get_inactive_time(NULL) < 500 || ui_scroll_busy(750)) return;
    xtouch_settings_flush();
}

void xtouch_settings_loadSettings(bool runtime = false)
{
    if (!xtouch_storage_has(xtouch_paths_settings))
    {
        DynamicJsonDocument doc(256);
        xtouch_language_set(2);
        strlcpy(xTouchConfig.xTouchPortalLanguage,"en",sizeof(xTouchConfig.xTouchPortalLanguage));
        xTouchConfig.xTouchBrowserTimezoneSync = true;
        xTouchConfig.xTouchBacklightLevel = 200;
        xTouchConfig.xTouchTFTOFFValue = 15;
        xTouchConfig.xTouchTFTInvert = false;
        xTouchConfig.xTouchPrinterPickerList = false;
        xTouchConfig.xTouchOTAEnabled = true;
        xTouchConfig.xTouchWakeOnPrint = true;
        xTouchConfig.xTouchSoundVolume=20;
        xTouchConfig.xTouchStatusLed=true;
        xTouchConfig.xTouchChamberSensorReadingDiff = 0;
        xTouchConfig.xTouchChamberSensorEnabled = false;
        xTouchConfig.xTouchAuxFanEnabled = false;
        xTouchConfig.xTouchChamberFanEnabled = false;
        xtouch_settings_save(true);
    }

    const bool previousInvert=xTouchConfig.xTouchTFTInvert;
    DynamicJsonDocument settings = xtouch_storage_read(xtouch_paths_settings, 2048);
    strlcpy(xTouchConfig.xTouchTimezone,settings["timezone"]|"",sizeof(xTouchConfig.xTouchTimezone));
    strlcpy(xTouchConfig.xTouchTimezoneName,settings["timezoneName"]|"",sizeof(xTouchConfig.xTouchTimezoneName));
    strlcpy(xTouchConfig.xTouchNtpServer,settings["ntpServer"]|"ntp.aliyun.com",sizeof(xTouchConfig.xTouchNtpServer));
    xTouchConfig.xTouchBrowserTimezoneSync=settings["browserTimezoneSync"]|true;
    const char *portalLanguage=settings["portalLanguage"]|"en";
    strlcpy(xTouchConfig.xTouchPortalLanguage,xtouch_language_index(portalLanguage)<0?"en":portalLanguage,sizeof(xTouchConfig.xTouchPortalLanguage));
    xtouch_language_set_code(settings["language"] | "en");

    xTouchConfig.xTouchBacklightLevel = settings.containsKey("backlight") ? settings["backlight"].as<int>() : 200;
    xTouchConfig.xTouchTFTOFFValue = settings.containsKey("tftOff") ? settings["tftOff"].as<int>() : 15;
    xTouchConfig.xTouchTFTInvert = settings.containsKey("tftInvert") ? settings["tftInvert"].as<bool>() : false;
    xTouchConfig.xTouchPrinterPickerList = settings["printerPickerList"] | false;
    xTouchConfig.xTouchOTAEnabled = settings.containsKey("ota") ? settings["ota"].as<bool>() : true;
    xTouchConfig.xTouchWakeOnPrint = settings.containsKey("wop") ? settings["wop"].as<bool>() : true;
    xTouchConfig.xTouchSoundVolume=constrain(settings["soundVolume"]|20,0,100);
    xTouchConfig.xTouchTouchSound=settings["touchSound"]|false;xTouchConfig.xTouchWarningSound=settings["warningSound"]|false;xTouchConfig.xTouchStatusLed=settings["statusLed"]|true;
    xTouchConfig.xTouchDeepSleepAfter=settings["deepSleepAfter"]|0;
    xTouchConfig.xTouchChamberSensorReadingDiff = settings.containsKey("chamberTempDiff") ? settings["chamberTempDiff"].as<int8_t>() : 0;
    xTouchConfig.xTouchChamberSensorEnabled = settings.containsKey("chamberTemp") ? settings["chamberTemp"].as<bool>() : false;
    xTouchConfig.xTouchAuxFanEnabled = settings.containsKey("auxFan") ? settings["auxFan"].as<bool>() : false;
    xTouchConfig.xTouchChamberFanEnabled = settings.containsKey("chamberFan") ? settings["chamberFan"].as<bool>() : false;

    if(!runtime)xtouch_screen_setupTFTFlip();
    if(!runtime||!xtouch_screen_touchFromPowerOff)xtouch_screen_setBrightness(xTouchConfig.xTouchBacklightLevel);
    if(!runtime||previousInvert!=xTouchConfig.xTouchTFTInvert)xtouch_screen_invertColors();
}

#endif
