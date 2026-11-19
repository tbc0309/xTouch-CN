#include "features/filename_font.h"
#if (XTOUCH_FILENAME_PUNCTUATION && !XTOUCH_FILENAME_COMMON_ZH) && XTOUCH_FILENAME_FONT_PIXELS == 28 && !XTOUCH_UI_HIDPI_ASSETS
#include <lvgl.h>
static bool no_glyph(const lv_font_t *f, lv_font_glyph_dsc_t *g, uint32_t c, uint32_t n) { (void)f; (void)g; (void)c; (void)n; return false; }
const lv_font_t xtouch_filename_ui_captions_28_small = {.get_glyph_dsc=no_glyph, .line_height=34, .base_line=6, .fallback=&XTOUCH_FILENAME_COMMON_NEXT_FONT};


#endif
