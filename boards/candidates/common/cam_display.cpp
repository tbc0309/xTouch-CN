#include "board.h"
#include "spi_transport.h"
#include "touch_contact.h"
#include "cam_pins.h"
#include "platform/idf/i2c.h"
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_intr_alloc.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7796.h>
namespace {
CandidateSpiTransport display;
TouchContact contact;
#if XTOUCH_CAM_PANEL_INCH10 == 28
volatile bool touchPending = true;
void IRAM_ATTR touchInterrupt(void *) { touchPending = true; }
#endif
bool touchReady;
constexpr uint8_t touchAddress = XTOUCH_CAM_PANEL_INCH10 == 28 ? 0x1a : 0x38;
bool expanderWrite(uint8_t reg, uint8_t value) {
    const uint8_t bytes[] = {reg, value};
    return xtouch_i2c.transfer(0x24, bytes, 2, nullptr, 0, 100) == ESP_OK;
}
bool read(uint16_t reg, unsigned regBytes, uint8_t *data, size_t count) {
    const uint8_t address[] = {uint8_t(reg >> 8), uint8_t(reg)};
    return xtouch_i2c.transfer(touchAddress, address + 2 - regBytes, regBytes, data, count, 100) == ESP_OK;
}
bool acknowledge() {
    const uint8_t bytes[] = {0xd0, 0x05, 0};
    return xtouch_i2c.transfer(touchAddress, bytes, sizeof(bytes), nullptr, 0, 100) == ESP_OK;
}
} // namespace
bool board_display_begin() {
    if (!xtouch_i2c.begin(8, 7, 400000) || !expanderWrite(0x02, 0xff) ||
        !expanderWrite(0x03, 0x00))
        return false;
    delay(10);
    if (!expanderWrite(0x03, 0x67))
        return false;
    delay(100);
    if (xtouch_i2c.probe(touchAddress, 100) != ESP_OK)
        return false;
    touchReady = true;
#if XTOUCH_CAM_PANEL_INCH10 == 28
    gpio_config_t irq{};
    irq.pin_bit_mask = 1ULL << BSP_LCD_TOUCH_INT;
    irq.mode = GPIO_MODE_INPUT;
    irq.pull_up_en = GPIO_PULLUP_ENABLE;
    irq.intr_type = GPIO_INTR_NEGEDGE;
    if (gpio_config(&irq) != ESP_OK) return false;
    const auto irqResult = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if ((irqResult != ESP_OK && irqResult != ESP_ERR_INVALID_STATE) ||
        gpio_isr_handler_add(BSP_LCD_TOUCH_INT, touchInterrupt, nullptr) != ESP_OK)
        return false;
#endif
    spi_bus_config_t spi{};
    spi.sclk_io_num = BSP_LCD_PCLK;
    spi.mosi_io_num = BSP_LCD_DATA0;
    spi.miso_io_num = -1;
    spi.quadwp_io_num = -1;
    spi.quadhd_io_num = -1;
    spi.max_transfer_sz = XTOUCH_DISPLAY_WIDTH * 16 * 2;
    if (spi_bus_initialize(SPI2_HOST, &spi, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    esp_lcd_panel_io_spi_config_t config{};
    config.cs_gpio_num = BSP_LCD_CS;
    config.dc_gpio_num = BSP_LCD_DC;
    config.pclk_hz = 40000000;
    config.trans_queue_depth = 1;
    config.lcd_cmd_bits = 8;
    config.lcd_param_bits = 8;
    esp_lcd_panel_io_handle_t io;
    if (esp_lcd_new_panel_io_spi(SPI2_HOST, &config, &io) != ESP_OK)
        return false;
    esp_lcd_panel_dev_config_t cfg{};
    cfg.reset_gpio_num = GPIO_NUM_NC;
    cfg.bits_per_pixel = 16;
    cfg.rgb_ele_order =
        XTOUCH_CAM_PANEL_INCH10 == 28 ? LCD_RGB_ELEMENT_ORDER_RGB : LCD_RGB_ELEMENT_ORDER_BGR;
    esp_lcd_panel_handle_t panel;
#if XTOUCH_CAM_PANEL_INCH10 == 28
    const auto result = esp_lcd_new_panel_st7789(io, &cfg, &panel);
#else
    const auto result = esp_lcd_new_panel_st7796(io, &cfg, &panel);
#endif
    return result == ESP_OK && esp_lcd_panel_reset(panel) == ESP_OK &&
           esp_lcd_panel_init(panel) == ESP_OK && esp_lcd_panel_swap_xy(panel, true) == ESP_OK &&
           esp_lcd_panel_mirror(panel, false, true) == ESP_OK &&
           esp_lcd_panel_invert_color(panel, true) == ESP_OK &&
           esp_lcd_panel_disp_on_off(panel, true) == ESP_OK && display.begin(panel, io);
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    display.draw(x, y, w, h, pixels);
}
void board_flush() { display.flush(); }
void board_brightness(uint8_t value) {
    expanderWrite(0x05, static_cast<unsigned>(value) * 100 / 255);
}
void board_invert(bool value) { display.invert(!value); }
bool board_display_sleep(bool sleeping) { return display.sleep(sleeping); }
bool board_touch(int &x, int &y) {
    if (!touchReady)
        return contact.read(x, y);
    uint8_t data[6]{};
    unsigned count;
    int tx = 0, ty = 0;
#if XTOUCH_CAM_PANEL_INCH10 == 28
    if (!touchPending && gpio_get_level(BSP_LCD_TOUCH_INT) != 0) return contact.read(x,y);
    touchPending = false;
    if (!read(0xd005, 2, data, 1))
        return contact.read(x, y);
    if (data[0] & 0x80) {
        acknowledge();
        return contact.read(x, y);
    } // Key report is not a pointer release.
    count = data[0] & 15;
    if (count > 5) {
        acknowledge();
        return contact.read(x, y);
    }
    if (count && !read(0xd000, 2, data, 5))
        return contact.read(x, y);
    if (!acknowledge())
        return contact.read(x, y);
    if (count) {
        tx = (int(data[1]) << 4) | (data[3] >> 4);
        ty = (int(data[2]) << 4) | (data[3] & 15);
    }
#else
    if (!read(2, 1, data, 5))
        return contact.read(x, y);
    count = data[0] & 15;
    if (count > 2)
        return contact.read(x, y);
    if (count) {
        tx = ((data[1] & 15) << 8) | data[2];
        ty = ((data[3] & 15) << 8) | data[4];
    }
#endif
    if (count && (tx >= XTOUCH_DISPLAY_HEIGHT || ty >= XTOUCH_DISPLAY_WIDTH))
        return contact.read(x, y);
    return contact.report(count != 0, ty, XTOUCH_DISPLAY_HEIGHT - 1 - tx, x, y);
}
