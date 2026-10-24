#ifndef _XLCD_SENSORS_CHAMBER_TEMP
#define _XLCD_SENSORS_CHAMBER_TEMP

#ifdef XTOUCH_WAVESHARE
inline void xtouch_chamber_timer_init() {}
inline void xtouch_chamber_timer_create() {}
inline void xtouch_chamber_timer_start() {}
inline void xtouch_chamber_timer_stop() {}
#else
#include <OneWire.h>
#include <DallasTemperature.h>

#define XTOUCH_CHAMBER_TEMP_PIN 22

// Setup a temperatureSensorsOneWire instance to communicate with any temperatureSensorsOneWire devices
OneWire temperatureSensorsOneWire(XTOUCH_CHAMBER_TEMP_PIN);

// Pass our temperatureSensorsOneWire reference to Dallas Temperature sensor
DallasTemperature xtouch_chamber_sensors(&temperatureSensorsOneWire);

lv_timer_t *xtouch_chambertemp_requestTemperaturesTimer;

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
    if(raw==DEVICE_DISCONNECTED_C||raw<-55||raw>125){bambuStatus.chamber_valid=false;xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP,0);xtouch_chamber_sensors.requestTemperatures();return;}
    int temperatureC = raw + xTouchConfig.xTouchChamberSensorReadingDiff;
    bambuStatus.chamber_valid=true;
    bambuStatus.chamber_temper = temperatureC;
    xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP, temperatureC);
    xtouch_chamber_sensors.requestTemperatures();
}

bool xtouch_chamber_started = false;
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

#endif // CYD external chamber sensor
#endif
