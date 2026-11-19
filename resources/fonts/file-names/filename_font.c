#include "features/filename_font.h"
#if XTOUCH_FILENAME_COMMON_ZH || XTOUCH_FILENAME_PUNCTUATION
/* One UI-thread-owned descriptor; glyph bitmaps stay in the existing UI font. */
const lv_font_t *xtouch_filename_common_font(void) {
    static lv_font_t combined;
    static bool ready;
    if (!ready) {
        combined = XTOUCH_FILENAME_BASE_FONT;
        /* Preserve the file browser's established line metrics. */
#if XTOUCH_FILENAME_FONT_PIXELS == 14
        combined.line_height = 17;
        combined.base_line = 4;
#else
        combined.line_height = 34;
        combined.base_line = 6;
#endif
#if XTOUCH_FILENAME_COMMON_ZH
        combined.fallback = &XTOUCH_FILENAME_COMMON_FONT;
#else
        combined.fallback = &XTOUCH_FILENAME_CAPTION_FONT;
#endif
        ready = true;
    }
    return &combined;
}
#endif
