#include "features/indicator/board_output.h"
#include "features/config.h"

#if XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED
#error "An RGB-capable board must provide its indicator driver"
#endif

bool xtouch_board_indicator_begin() {
    return false;
}

bool xtouch_board_indicator_rgb(uint8_t, uint8_t, uint8_t) {
    return false;
}
