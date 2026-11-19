#include "indicator.h"
#include "board_output.h"
#include "features/config.h"
#include "platform/idf/runtime.h"
namespace {
bool ready = false;
uint32_t last = UINT32_MAX;
} // namespace
bool xtouch_indicator_available() {
    return ready;
}
uint32_t xtouch_indicator_color() {
    return last == UINT32_MAX ? 0 : last;
}
void xtouch_indicator_begin() {
#if XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED
    ready = xtouch_board_indicator_begin();
    last = UINT32_MAX;
#else
    ready = false;
#endif
}
void xtouch_indicator_poll(bool connected, bool warning, bool sleeping, bool enabled) {
#if XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED
    if (!ready)
        return;
    bool on = enabled && !sleeping, blink = (millis() % 1600) < 400;
    uint8_t red = on && warning && blink ? 255 : 0, green = on && (connected || warning) ? 255 : 0,
            blue = on && !connected && !warning && blink ? 255 : 0;
    uint32_t rgb = (uint32_t(red) << 16) | (uint32_t(green) << 8) | blue;
    if (last == rgb)
        return;
    if (xtouch_board_indicator_rgb(red, green, blue))
        last = rgb;
    else
        ready = false;
#else
    (void)connected;
    (void)warning;
    (void)sleeping;
    (void)enabled;
#endif
}
