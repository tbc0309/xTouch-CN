#pragma once
#include <stdint.h>

struct XTouchDisplayStats {
    bool available = false;
    bool dma = false;
    bool includesFirstTile = false;
    uint32_t flushes = 0, frames = 0, transactions = 0, wireBytes = 0;
    uint32_t lastFrameUs = 0, maxFrameUs = 0, bufferBytes = 0, failedTransfers = 0;
};

/* With includesFirstTile, timing starts at LVGL RENDER_START and ends at final
 * wire completion. Otherwise it starts at the first flush submission.
 * Unsupported ports return available=false; timing is not a frame-rate claim. */
XTouchDisplayStats xtouch_board_display_stats();
