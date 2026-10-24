#include "capabilities.h"
#include "config.h"
#include "diagnostics/loop_timing.h"
#include "diagnostics/mqtt_timing.h"
#include "xtouch/types.h"
#include "sound/events.h"
#include "indicator/indicator.h"
#include "power/deep_sleep.h"
#include "camera/camera.h"
void xtouch_feature_diagnostics(JsonDocument &d){
 auto mqtt=d.createNestedObject("mqttTiming");
 mqtt["attempts"]=xtouch_mqtt_timing.attempts;mqtt["transportMs"]=xtouch_mqtt_timing.transportMs;
 mqtt["loginMs"]=xtouch_mqtt_timing.loginMs;mqtt["subscribeMs"]=xtouch_mqtt_timing.subscribeMs;mqtt["totalMs"]=xtouch_mqtt_timing.totalMs;
 mqtt["cpuBeforeMHz"]=xtouch_mqtt_timing.cpuBeforeMHz;mqtt["cpuWorkMHz"]=xtouch_mqtt_timing.cpuWorkMHz;
 mqtt["state"]=xtouch_mqtt_timing.state;mqtt["transportReady"]=xtouch_mqtt_timing.transportReady;mqtt["connected"]=xtouch_mqtt_timing.connected;
 const char *phases[]={"gui","portal","mqtt","mqttConnect","mqttReceive","preview","sd"};auto timing=d.createNestedObject("loopTiming");for(unsigned n=0;n<XTOUCH_PHASE_COUNT;++n)timing[phases[n]]=xtouch_loop_timing.maximum[n];timing["lastPhase"]=xtouch_loop_timing.lastAt?phases[xtouch_loop_timing.lastPhase]:"none";timing["lastMs"]=xtouch_loop_timing.lastMs;timing["lastAgeMs"]=xtouch_loop_timing.lastAt?millis()-xtouch_loop_timing.lastAt:0;
 auto c=d.createNestedObject("capabilities");auto audio=xtouch_sound_status();
 c["audio"]=bool(XTOUCH_FEATURE_AUDIO);c["indicator"]=bool(XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED);c["deepSleep"]=bool(XTOUCH_FEATURE_DEEP_SLEEP);c["deepTouchWake"]=bool(XTOUCH_FEATURE_DEEP_SLEEP && XTOUCH_HAS_DEEP_TOUCH_WAKE);
 c["camera"]=bool(XTOUCH_FEATURE_CAMERA);c["printPreview"]=bool(XTOUCH_FEATURE_PRINT_PREVIEW);c["screenWidth"]=XTOUCH_DISPLAY_WIDTH;c["screenHeight"]=XTOUCH_DISPLAY_HEIGHT;c["iconStyle"]=XTOUCH_ICON_STYLE?"mdi":"original";
#if XTOUCH_FEATURE_CAMERA
 c["cameraAvailable"]=xtouch_camera_available();
#else
 c["cameraAvailable"]=false;
#endif
 auto s=d.createNestedObject("sound");s["volume"]=xTouchConfig.xTouchSoundVolume;s["playing"]=audio.playing;s["sleeping"]=audio.sleeping;s["played"]=audio.played;s["dropped"]=audio.dropped;s["error"]=audio.error;
}
