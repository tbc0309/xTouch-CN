#pragma once
#ifndef XTOUCH_FEATURE_EXTERNAL_CHAMBER
#define XTOUCH_FEATURE_EXTERNAL_CHAMBER 0
#endif
#ifndef XTOUCH_FEATURE_XIAOMI_BLE
#define XTOUCH_FEATURE_XIAOMI_BLE 0
#endif
/* Hardware facts are overridable by a new port; shared features never guess pins. */
#ifndef XTOUCH_DEEP_TOUCH_PIN
#if defined(XTOUCH_BOARD_S3)
#define XTOUCH_DEEP_TOUCH_PIN 4
#elif defined(__XTOUCH_SCREEN_28__)
#define XTOUCH_DEEP_TOUCH_PIN 36
#else
#define XTOUCH_DEEP_TOUCH_PIN -1
#endif
#endif
#ifndef XTOUCH_DEEP_WAKE_PIN
#define XTOUCH_DEEP_WAKE_PIN XTOUCH_DEEP_TOUCH_PIN
#endif
#ifndef XTOUCH_HAS_DEEP_TOUCH_WAKE
#ifdef XTOUCH_BOARD_P4
#define XTOUCH_HAS_DEEP_TOUCH_WAKE 1
#else
#define XTOUCH_HAS_DEEP_TOUCH_WAKE (XTOUCH_DEEP_TOUCH_PIN>=0)
#endif
#endif
#ifndef XTOUCH_HAS_RGB_LED
#ifdef __XTOUCH_SCREEN_28__
#define XTOUCH_HAS_RGB_LED 1
#else
#define XTOUCH_HAS_RGB_LED 0
#endif
#endif
/* Battery power key is readable only on boards which wire it to the MCU. */
#ifndef XTOUCH_SLEEP_BUTTON_PIN
#ifdef XTOUCH_BOARD_S3
#define XTOUCH_SLEEP_BUTTON_PIN 6
#elif defined(XTOUCH_BOARD_P4)
#define XTOUCH_SLEEP_BUTTON_PIN 35
#else
#define XTOUCH_SLEEP_BUTTON_PIN -1
#endif
#endif
#ifndef XTOUCH_FEATURE_BLE_PROVISIONING
#define XTOUCH_FEATURE_BLE_PROVISIONING 0
#endif
#if XTOUCH_FEATURE_BLE_PROVISIONING && !defined(XTOUCH_WAVESHARE)
#error "BLE provisioning is available only on S3/P4 builds; CYD retains dual OTA slots"
#endif
/* Build-time feature switches. Runtime preferences never enable an omitted module. */
#ifndef XTOUCH_PORTAL_RECLAIM_TLS_ON_READ
#ifdef XTOUCH_WAVESHARE
#define XTOUCH_PORTAL_RECLAIM_TLS_ON_READ 0
#else
#define XTOUCH_PORTAL_RECLAIM_TLS_ON_READ 1
#endif
#endif
#ifndef XTOUCH_FEATURE_AUDIO
#if defined(XTOUCH_WAVESHARE) || defined(__XTOUCH_SCREEN_28__)
#define XTOUCH_FEATURE_AUDIO 1
#else
#define XTOUCH_FEATURE_AUDIO 0
#endif
#endif
#ifndef XTOUCH_FEATURE_DEEP_SLEEP
#define XTOUCH_FEATURE_DEEP_SLEEP 1
#endif
#ifndef XTOUCH_FEATURE_INDICATOR
#define XTOUCH_FEATURE_INDICATOR 1
#endif
#ifndef XTOUCH_FEATURE_CAMERA
#ifdef XTOUCH_WAVESHARE
#define XTOUCH_FEATURE_CAMERA 1
#else
#define XTOUCH_FEATURE_CAMERA 0
#endif
#endif
#ifndef XTOUCH_FEATURE_PRINT_PREVIEW
#define XTOUCH_FEATURE_PRINT_PREVIEW XTOUCH_FEATURE_CAMERA
#endif
#ifndef XTOUCH_DISPLAY_WIDTH
#ifdef XTOUCH_BOARD_P4
#define XTOUCH_DISPLAY_WIDTH 800
#define XTOUCH_DISPLAY_HEIGHT 480
#else
#define XTOUCH_DISPLAY_WIDTH 320
#define XTOUCH_DISPLAY_HEIGHT 240
#endif
#endif
#ifndef XTOUCH_DISPLAY_HEIGHT
#error Define XTOUCH_DISPLAY_HEIGHT together with XTOUCH_DISPLAY_WIDTH
#endif

#ifndef XTOUCH_ICON_STYLE
#if XTOUCH_DISPLAY_WIDTH>=480 && XTOUCH_DISPLAY_HEIGHT>=320
#define XTOUCH_ICON_STYLE 1
#else
#define XTOUCH_ICON_STYLE 0
#endif
#endif

// Password/code cloud authentication is supported by the native S3/P4 transport.
#ifndef XTOUCH_FEATURE_CLOUD_LOGIN
#ifdef XTOUCH_WAVESHARE
#define XTOUCH_FEATURE_CLOUD_LOGIN 1
#else
#define XTOUCH_FEATURE_CLOUD_LOGIN 0
#endif
#endif

// Token entry and account sign-in are independent; authenticated sessions share transport.
#ifndef XTOUCH_FEATURE_CLOUD_TOKEN
#define XTOUCH_FEATURE_CLOUD_TOKEN 1
#endif

#ifndef XTOUCH_DEEP_WAKE_BUTTON_NAME
#define XTOUCH_DEEP_WAKE_BUTTON_NAME ""
#endif
