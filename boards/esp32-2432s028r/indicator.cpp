#include "features/indicator/board_output.h"
#include <driver/gpio.h>
bool xtouch_board_indicator_begin() {
    gpio_config_t config{};
    config.pin_bit_mask = (1ULL << 4) | (1ULL << 16) | (1ULL << 17);
    config.mode = GPIO_MODE_OUTPUT;
    return gpio_config(&config) == ESP_OK && xtouch_board_indicator_rgb(0, 0, 0);
}
bool xtouch_board_indicator_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    return gpio_set_level(GPIO_NUM_4, !red) == ESP_OK &&
           gpio_set_level(GPIO_NUM_16, !green) == ESP_OK &&
           gpio_set_level(GPIO_NUM_17, !blue) == ESP_OK;
}
