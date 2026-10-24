#pragma once
#include "config.h"
/* Board-to-feature mapping belongs here, never in application pages.
 * New ports can define these capabilities in their build configuration. */
#ifndef XTOUCH_UI_HIDPI_ASSETS
#if XTOUCH_DISPLAY_WIDTH>=480 && XTOUCH_DISPLAY_HEIGHT>=320
#define XTOUCH_UI_HIDPI_ASSETS 1
#else
#define XTOUCH_UI_HIDPI_ASSETS 0
#endif
#endif
#if XTOUCH_DISPLAY_WIDTH!=320 || XTOUCH_DISPLAY_HEIGHT!=240
#ifndef XTOUCH_UI_NATIVE_CANVAS
#define XTOUCH_UI_NATIVE_CANVAS 1
#endif
#endif
#if defined(XTOUCH_WAVESHARE)
#ifndef XTOUCH_HAS_CAMERA
#define XTOUCH_HAS_CAMERA XTOUCH_FEATURE_CAMERA
#endif
#endif

#ifndef XTOUCH_UI_LVGL_BYTES
#if defined(XTOUCH_WAVESHARE)
#define XTOUCH_UI_LVGL_BYTES (256U * 1024U)
#else
#define XTOUCH_UI_LVGL_BYTES (40U * 1024U)
#endif
#endif

#ifndef XTOUCH_HAS_PRINT_PREVIEW
#ifdef XTOUCH_WAVESHARE
#define XTOUCH_HAS_PRINT_PREVIEW XTOUCH_FEATURE_PRINT_PREVIEW
#else
#define XTOUCH_HAS_PRINT_PREVIEW 0
#endif
#endif

/* The picker draws rows directly, without a widget or framebuffer per device. */
#ifndef XTOUCH_UI_PICTURE_PICKER
#define XTOUCH_UI_PICTURE_PICKER 1
#endif
