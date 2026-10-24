#include <Arduino.h>
#include "types.h"

void xtouch_ams_parse_tray_now(const char *tray_now)
{
    if (!tray_now || strlen(tray_now) == 0)
    {
        return;
    }
    else
    {
        int tray_now_int = atoi(tray_now);
        if (tray_now_int >= 0 && tray_now_int < XTOUCH_MAX_AMS_SLOTS)
        {
            bambuStatus.m_ams_id = tray_now_int >> 2;
            bambuStatus.m_tray_id = tray_now_int & 0x3;
        }
        else if (tray_now_int == 255)
        {
            bambuStatus.m_ams_id = 0;
            bambuStatus.m_tray_id = 0;
        }
    }
}

void xtouch_ams_parse_status(int ams_status)
{
    bambuStatus.ams_status_sub = ams_status & 0xFF;
    int ams_status_main_int = (ams_status & 0xFF00) >> 8;
    if (ams_status_main_int == AMS_STATUS_MAIN_IDLE)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_IDLE;
    }
    else if (ams_status_main_int == AMS_STATUS_MAIN_FILAMENT_CHANGE)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_FILAMENT_CHANGE;
    }
    else if (ams_status_main_int == AMS_STATUS_MAIN_RFID_IDENTIFYING)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_RFID_IDENTIFYING;
    }
    else if (ams_status_main_int == AMS_STATUS_MAIN_ASSIST)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_ASSIST;
    }
    else if (ams_status_main_int == AMS_STATUS_MAIN_CALIBRATION)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_CALIBRATION;
    }
    else if (ams_status_main_int == AMS_STATUS_MAIN_SELF_CHECK)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_SELF_CHECK;
    }
    else if (ams_status_main_int == AMS_STATUS_MAIN_DEBUG)
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_DEBUG;
    }
    else
    {
        bambuStatus.ams_status_main = AMS_STATUS_MAIN_UNKNOWN;
    }
}
