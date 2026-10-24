#ifndef _XLCD_GLOBALS
#define _XLCD_GLOBALS

void xtouch_globals_init()
{
    // No active AMS tray is known until the printer reports one.
    bambuStatus.m_tray_now = 255;
    bambuStatus.m_tray_tar = 255;
    memset(bambuStatus.ams_bound_extruder,255,sizeof(bambuStatus.ams_bound_extruder));
    bambuStatus.nozzle_loaded[0]=bambuStatus.nozzle_loaded[1]=255;
    controlMode.inc = 1;
    controlMode.axis = ControlAxisXY;
}

#endif
