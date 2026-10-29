#include "features/indicator/board_output.h"
#include "port_profile.h"
#include <driver/gpio.h>
#include <led_strip.h>
namespace {
led_strip_handle_t strip;
}
bool xtouch_board_indicator_begin() {
    const auto &p = XTouchPort::profile();
    if (p.indicator < 0)
        return false;
    if (!p.indicatorRgb) {
        return gpio_set_direction(gpio_num_t(p.indicator), GPIO_MODE_OUTPUT) == ESP_OK;
    }
    if (strip)
        return true;
    led_strip_config_t c{};
    c.strip_gpio_num = p.indicator;
    c.max_leds = 1;
    c.led_model = LED_MODEL_WS2812;
    c.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    led_strip_rmt_config_t r{};
    r.clk_src = RMT_CLK_SRC_DEFAULT;
    r.resolution_hz = 10000000;
    r.mem_block_symbols = 64;
    return led_strip_new_rmt_device(&c, &r, &strip) == ESP_OK && led_strip_clear(strip) == ESP_OK;
}
bool xtouch_board_indicator_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    const auto &p = XTouchPort::profile();
    if (p.indicator < 0)
        return false;
    if (!p.indicatorRgb)
        return gpio_set_level(gpio_num_t(p.indicator), red || green || blue) == ESP_OK;
    return strip &&
           led_strip_set_pixel(strip, 0, unsigned(red) * 24 / 255, unsigned(green) * 24 / 255,
                               unsigned(blue) * 24 / 255) == ESP_OK &&
           led_strip_refresh(strip) == ESP_OK;
}
