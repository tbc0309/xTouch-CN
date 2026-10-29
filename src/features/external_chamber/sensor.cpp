#include "features/config.h"
#if XTOUCH_FEATURE_EXTERNAL_CHAMBER
#include "sensor.h"
#include "ui/ui.h"
#include "ui/ui_msgs.h"
static void notify(int value){XTOUCH_MESSAGE_DATA event={};event.data=value;lv_msg_send(XTOUCH_ON_CHAMBER_TEMP,&event);}
/* Optional CYD GPIO22 DS18B20 adapter; included only when selected at build time. */
#include "platform/idf/onewire_temperature.h"
static NativeOneWireTemperature xtouch_chamber_sensors;
#define DEVICE_DISCONNECTED_C -127

static lv_timer_t *xtouch_chambertemp_requestTemperaturesTimer;

void xtouch_chamber_requestTemperatures(lv_timer_t *timer);

void xtouch_chamber_timer_create()
{
    if(!xtouch_chambertemp_requestTemperaturesTimer)
        xtouch_chambertemp_requestTemperaturesTimer = lv_timer_create(xtouch_chamber_requestTemperatures, 2500, NULL);
    else lv_timer_resume(xtouch_chambertemp_requestTemperaturesTimer);
}

void xtouch_chamber_requestTemperatures(lv_timer_t *timer)
{
    if(!xtouch_bblp_is_p1Series()||!xTouchConfig.xTouchChamberSensorEnabled)return;
    float raw=xtouch_chamber_sensors.getTempCByIndex(0);
    if(raw==DEVICE_DISCONNECTED_C||raw<-55||raw>125){bambuStatus.chamber_valid=false;notify(0);xtouch_chamber_sensors.requestTemperatures();return;}
    int temperatureC = raw + xTouchConfig.xTouchChamberSensorReadingDiff;
    bambuStatus.chamber_valid=true;
    bambuStatus.chamber_temper = temperatureC;
    notify(temperatureC);
    xtouch_chamber_sensors.requestTemperatures();
}

static bool xtouch_chamber_started = false;
void xtouch_chamber_timer_start()
{
    if (!xtouch_chamber_started)
    {
        xtouch_chamber_sensors.begin();
        xtouch_chamber_sensors.setWaitForConversion(false);
        xtouch_chamber_sensors.requestTemperatures();
        xtouch_chamber_started = true;
    }
    xtouch_chamber_timer_create();
}

void xtouch_chamber_timer_stop()
{
    if(xtouch_chambertemp_requestTemperaturesTimer)lv_timer_pause(xtouch_chambertemp_requestTemperaturesTimer);
}

void xtouch_chamber_timer_init()
{
    if (xTouchConfig.xTouchChamberSensorEnabled)
    {
        xtouch_chamber_timer_start();
    }
    else
    {
        if (xtouch_chamber_started)
        {

            xtouch_chamber_timer_stop();
        }
    }
}


#endif
