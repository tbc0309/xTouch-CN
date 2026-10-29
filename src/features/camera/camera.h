#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void xtouch_camera_open(void);
bool xtouch_camera_available(void);
void xtouch_camera_poll(void);
void xtouch_camera_suspend(void);
typedef struct { bool open, running; unsigned frames, connections, state, width, height, lastFrameAgeMs; uint32_t freeInternal, largestInternal; unsigned closeReason; } XTouchCameraStatus;
XTouchCameraStatus xtouch_camera_status(void);
#ifdef __cplusplus
}
#endif
