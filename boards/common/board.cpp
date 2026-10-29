#include "board.h"
#include "features/diagnostics/display_stats.h"
#include "platform/idf/filesystem.h"
XTouchDisplayStats __attribute__((weak)) xtouch_board_display_stats() {
    return {};
}
bool __attribute__((weak)) board_draw_buffers(uint16_t **, uint16_t **, uint32_t *) {
    return false;
}
bool __attribute__((weak)) board_pixels_async(lv_display_t *, int, int, int, int,
                                              const uint16_t *) {
    return false;
}
bool board_card_begin() {
    return SD_MMC.begin();
}

void __attribute__((weak)) board_display_ready(lv_display_t *) {}
