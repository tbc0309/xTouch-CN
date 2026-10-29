#include "board.h"
#include "bsp_touch_packets.h"
#include "native_spi_config.h"
#include "platform/idf/i2c.h"
#include "port_st7796_init.h"
#include "spi_transport.h"
#include "touch_contact.h"
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7796.h>
namespace {
CandidateSpiTransport display;
TouchContact contact;
bool write(uint8_t device, uint8_t reg, uint8_t value) {
    const uint8_t bytes[] = {reg, value};
    return xtouch_i2c.transfer(device, bytes, 2, nullptr, 0) == ESP_OK;
}
bool read(uint8_t device, uint8_t reg, uint8_t *data, size_t length) {
    return xtouch_i2c.transfer(device, &reg, 1, data, length) == ESP_OK;
}
} // namespace
bool board_display_begin() {
    const auto &p = NativeCandidate::config();
    if (!xtouch_i2c.begin(p.sda, p.scl, 400000))
        return false;
    if (p.classicPower) {
        if (!write(0x20, 3, 0xf8) || !write(0x20, 1, 0))
            return false;
        delay(100);
        if (!write(0x20, 1, 7))
            return false;
        const uint8_t power[][2] = {{0x22, 6},  {0x27, 0x10}, {0x80, 1},  {0x90, 0},  {0x91, 0},
                                    {0x82, 18}, {0x92, 28},   {0x96, 10}, {0x97, 23}, {0x90, 0x31}};
        for (const auto &setting : power)
            if (!write(0x34, setting[0], setting[1]))
                return false;
    }
    spi_bus_config_t bus{};
    bus.sclk_io_num = p.clock;
    bus.mosi_io_num = p.mosi;
    bus.miso_io_num = p.miso;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = XTOUCH_DISPLAY_WIDTH * 16 * 2;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    esp_lcd_panel_io_spi_config_t ioConfig{};
    ioConfig.cs_gpio_num = gpio_num_t(p.cs);
    ioConfig.dc_gpio_num = gpio_num_t(p.dc);
    ioConfig.pclk_hz = 40000000;
    ioConfig.trans_queue_depth = 1;
    ioConfig.lcd_cmd_bits = 8;
    ioConfig.lcd_param_bits = 8;
    esp_lcd_panel_io_handle_t io;
    if (esp_lcd_new_panel_io_spi(SPI2_HOST, &ioConfig, &io) != ESP_OK)
        return false;
    esp_lcd_panel_dev_config_t cfg{};
    cfg.reset_gpio_num = gpio_num_t(p.reset);
    cfg.bits_per_pixel = 16;
    cfg.rgb_ele_order = p.bgr ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB;
    st7796_vendor_config_t vendor{};
    if (p.classicPower) {
        vendor.init_cmds = port_st7796_init;
        vendor.init_cmds_size = sizeof(port_st7796_init) / sizeof(port_st7796_init[0]);
        cfg.vendor_config = &vendor;
    }
    esp_lcd_panel_handle_t panel;
    const auto result = p.panel == NativeCandidate::Panel::St7789
                            ? esp_lcd_new_panel_st7789(io, &cfg, &panel)
                            : esp_lcd_new_panel_st7796(io, &cfg, &panel);
    if (result != ESP_OK || esp_lcd_panel_reset(panel) != ESP_OK ||
        esp_lcd_panel_init(panel) != ESP_OK || esp_lcd_panel_swap_xy(panel, p.swap) != ESP_OK ||
        esp_lcd_panel_mirror(panel, p.mirrorX, p.mirrorY) != ESP_OK ||
        esp_lcd_panel_invert_color(panel, p.invert) != ESP_OK ||
        esp_lcd_panel_disp_on_off(panel, true) != ESP_OK || !display.begin(panel, io))
        return false;
    delay(100);
    if (xtouch_i2c.probe(p.touchAddress, 100) != ESP_OK)
        return false;
    if (p.backlight >= 0) {
        ledc_timer_config_t timer{};
        timer.speed_mode = LEDC_LOW_SPEED_MODE;
        timer.duty_resolution = LEDC_TIMER_10_BIT;
        timer.timer_num = LEDC_TIMER_1;
        timer.freq_hz = 20000;
        timer.clk_cfg = LEDC_AUTO_CLK;
        ledc_channel_config_t channel{};
        channel.gpio_num = p.backlight;
        channel.speed_mode = LEDC_LOW_SPEED_MODE;
        channel.channel = LEDC_CHANNEL_0;
        channel.timer_sel = LEDC_TIMER_1;
        if (ledc_timer_config(&timer) != ESP_OK || ledc_channel_config(&channel) != ESP_OK)
            return false;
    }
    return true;
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    display.draw(x, y, w, h, pixels);
}
void board_flush() { display.flush(); }
void board_brightness(uint8_t value) {
    if (NativeCandidate::config().backlight < 0)
        return;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, static_cast<unsigned>(value) * 1023 / 255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
void board_invert(bool value) { display.invert(NativeCandidate::config().invert ^ value); }
bool board_display_sleep(bool sleeping) { return display.sleep(sleeping); }
bool board_touch(int &x, int &y) {
    const auto &p = NativeCandidate::config();
    uint8_t data[57]{};
    int tx = 0, ty = 0;
    bool down = false;
    if (p.touch == NativeCandidate::Touch::Tt21100) {
        if (xtouch_i2c.transfer(p.touchAddress, nullptr, 0, data, 2) != ESP_OK)
            return contact.read(x, y);
        const unsigned n = static_cast<unsigned>(data[0]) | (static_cast<unsigned>(data[1]) << 8);
        if (n < 7 || n > sizeof(data))
            return contact.read(x, y);
        if (xtouch_i2c.transfer(p.touchAddress, nullptr, 0, data, n) != ESP_OK)
            return contact.read(x, y);
        const auto e = CandidateTouch::tt21100(data, n, tx, ty);
        if (e == CandidateTouch::Event::Ignore)
            return contact.read(x, y);
        down = e == CandidateTouch::Event::Pressed;
    } else {
        if (!read(p.touchAddress, 2, data, 5))
            return contact.read(x, y);
        const unsigned n = data[0] & 15;
        if (n > 2)
            return contact.read(x, y);
        down = n != 0;
        tx = ((data[1] & 15) << 8) | data[2];
        ty = ((data[3] & 15) << 8) | data[4];
    }
    if (p.touchSwap)
        std::swap(tx, ty);
    if (p.touchMirrorX)
        tx = XTOUCH_DISPLAY_WIDTH - 1 - tx;
    if (p.touchMirrorY)
        ty = XTOUCH_DISPLAY_HEIGHT - 1 - ty;
    if (down && (tx < 0 || ty < 0 || tx >= XTOUCH_DISPLAY_WIDTH || ty >= XTOUCH_DISPLAY_HEIGHT))
        return contact.read(x, y);
    return contact.report(down, tx, ty, x, y);
}
