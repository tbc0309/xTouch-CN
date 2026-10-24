#pragma once
#include <atomic>
#include <cstdint>
// Runs only on the camera worker. No LVGL calls or references to UI objects.
struct CameraRtspSink {
 uint16_t *pixels;unsigned width,height;
 void (*publish)(unsigned,unsigned);
 void (*status)(int);
};
// Return value is an error caption only when stop is false (user exit wins).
int cameraRtspRun(const char *host,const char *code,std::atomic<bool> &stop,const CameraRtspSink &sink);
