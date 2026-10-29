#include "features/indicator/board_output.h"
#include <led_strip.h>
static led_strip_handle_t strip;
bool xtouch_board_indicator_begin() {
    if (!strip) {
        led_strip_config_t config{};
        config.strip_gpio_num = 40;
        config.max_leds = 1;
        config.led_model = LED_MODEL_WS2812;
        config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
        led_strip_rmt_config_t rmt{};
        rmt.clk_src = RMT_CLK_SRC_DEFAULT;
        rmt.resolution_hz = 10000000;
        rmt.mem_block_symbols = 64;
        if (led_strip_new_rmt_device(&config, &rmt, &strip) != ESP_OK)
            return false;
    }
    return led_strip_clear(strip) == ESP_OK;
}
bool xtouch_board_indicator_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    // One bright WS2812 needs much less current than the CYD's discrete LED.
    if (!strip)
        return false;
    return led_strip_set_pixel(strip, 0, unsigned(red) * 24 / 255, unsigned(green) * 24 / 255,
                               unsigned(blue) * 24 / 255) == ESP_OK &&
           led_strip_refresh(strip) == ESP_OK;
}
