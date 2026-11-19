#pragma once
#include "features/config.h"
#include <lvgl.h>

#ifndef XTOUCH_FILENAME_COMMON_ZH
#define XTOUCH_FILENAME_COMMON_ZH 0
#endif
#ifndef XTOUCH_FILENAME_PUNCTUATION
#define XTOUCH_FILENAME_PUNCTUATION 0
#endif

#if XTOUCH_DISPLAY_WIDTH >= 640 && XTOUCH_DISPLAY_HEIGHT >= 400
#define XTOUCH_FILENAME_FONT_PIXELS 28
#else
#define XTOUCH_FILENAME_FONT_PIXELS 14
#endif

#if XTOUCH_FILENAME_COMMON_ZH || XTOUCH_FILENAME_PUNCTUATION
#ifdef __cplusplus
extern "C" {
#endif
#if XTOUCH_FILENAME_FONT_PIXELS == 14
#define XTOUCH_FILENAME_COMMON_FONT xtouch_filename_common_zh_14
#define XTOUCH_FILENAME_CAPTION_FONT xtouch_filename_ui_captions_14
#define XTOUCH_FILENAME_PUNCTUATION_FONT xtouch_filename_punctuation_14
#if XTOUCH_UI_HIDPI_ASSETS
#define XTOUCH_FILENAME_BASE_FONT ui_native_body_14
#else
#define XTOUCH_FILENAME_BASE_FONT ui_font_text_14
#endif
#define XTOUCH_FILENAME_TERMINAL_FONT XTOUCH_FILENAME_BASE_FONT
#elif XTOUCH_UI_HIDPI_ASSETS
#define XTOUCH_FILENAME_COMMON_FONT xtouch_filename_common_zh_28
#define XTOUCH_FILENAME_CAPTION_FONT xtouch_filename_ui_captions_28
#define XTOUCH_FILENAME_PUNCTUATION_FONT xtouch_filename_punctuation_28
#define XTOUCH_FILENAME_BASE_FONT ui_font_text_14
#define XTOUCH_FILENAME_TERMINAL_FONT ui_font_text_14
#else
#define XTOUCH_FILENAME_COMMON_FONT xtouch_filename_common_zh_28_small
#define XTOUCH_FILENAME_CAPTION_FONT xtouch_filename_ui_captions_28_small
#define XTOUCH_FILENAME_PUNCTUATION_FONT xtouch_filename_punctuation_28_small
#define XTOUCH_FILENAME_BASE_FONT xtouch_files_font_28
#define XTOUCH_FILENAME_TERMINAL_FONT ui_font_text_28
#endif
LV_FONT_DECLARE(XTOUCH_FILENAME_BASE_FONT);
LV_FONT_DECLARE(XTOUCH_FILENAME_TERMINAL_FONT);
#if XTOUCH_FILENAME_COMMON_ZH
LV_FONT_DECLARE(XTOUCH_FILENAME_COMMON_FONT);
#endif
#if XTOUCH_FILENAME_PUNCTUATION
LV_FONT_DECLARE(XTOUCH_FILENAME_PUNCTUATION_FONT);
#if !XTOUCH_FILENAME_COMMON_ZH
LV_FONT_DECLARE(XTOUCH_FILENAME_CAPTION_FONT);
#endif
#define XTOUCH_FILENAME_COMMON_NEXT_FONT XTOUCH_FILENAME_PUNCTUATION_FONT
#else
#define XTOUCH_FILENAME_COMMON_NEXT_FONT XTOUCH_FILENAME_TERMINAL_FONT
#endif
const lv_font_t *xtouch_filename_common_font(void);
#ifdef __cplusplus
}
#endif
#endif

static inline const lv_font_t *xtouch_filename_font(const lv_font_t *ui_font) {
#if XTOUCH_FILENAME_COMMON_ZH || XTOUCH_FILENAME_PUNCTUATION
    (void)ui_font;
    return xtouch_filename_common_font();
#else
    return ui_font;
#endif
}
