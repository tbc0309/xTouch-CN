#include "board.h"
#include "port_profile.h"
#include "port_st7796_init.h"
#include "touch_contact.h"
#include "platform/idf/i2c.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_st7796.h>
#include <esp_log.h>
#include <freertos/semphr.h>
#include <algorithm>
#include <cstring>
namespace {
esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t io;
SemaphoreHandle_t complete;
uint16_t *tile;
bool pending;
constexpr unsigned Rows = 16;
bool done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(complete, &wake);
    return wake == pdTRUE;
}
} // namespace
bool board_display_begin() {
    const auto &p = XTouchPort::profile();
    if (!XTouchPort::preparePeripherals())
        return false;
    complete = xSemaphoreCreateBinary();
    tile = static_cast<uint16_t *>(
        heap_caps_malloc(p.width * Rows * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!complete || !tile)
        return false;
    spi_bus_config_t bus{};
    bus.sclk_io_num = p.clock;
    bus.mosi_io_num = p.mosi;
    bus.miso_io_num = p.miso;
    bus.quadwp_io_num = p.data2;
    bus.quadhd_io_num = p.data3;
    bus.max_transfer_sz = p.width * Rows * 2;
    if (spi_bus_initialize(spi_host_device_t(p.spiHost), &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    esp_lcd_panel_io_spi_config_t cfg{};
    cfg.cs_gpio_num = gpio_num_t(p.cs);
    cfg.dc_gpio_num = gpio_num_t(p.dc);
    cfg.pclk_hz = p.hz;
    cfg.spi_mode = p.spiMode;
    cfg.lcd_cmd_bits = 8;
    cfg.lcd_param_bits = 8;
    cfg.trans_queue_depth = 1;
    cfg.on_color_trans_done = done;
    if (esp_lcd_new_panel_io_spi(spi_host_device_t(p.spiHost), &cfg, &io) != ESP_OK)
        return false;
    esp_lcd_panel_dev_config_t dev{};
    dev.reset_gpio_num = gpio_num_t(p.reset);
    dev.bits_per_pixel = 16;
    dev.rgb_ele_order = p.bgr ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB;
    st7796_vendor_config_t vendor{};
    if (p.customSt7796) {
        vendor.init_cmds = port_st7796_init;
        vendor.init_cmds_size = sizeof(port_st7796_init) / sizeof(port_st7796_init[0]);
        dev.vendor_config = &vendor;
    }
    auto err = p.panel == XTouchPort::Panel::Ili9341  ? esp_lcd_new_panel_ili9341(io, &dev, &panel)
               : p.panel == XTouchPort::Panel::St7796 ? esp_lcd_new_panel_st7796(io, &dev, &panel)
                                                      : esp_lcd_new_panel_st7789(io, &dev, &panel);
    if (err != ESP_OK || esp_lcd_panel_reset(panel) != ESP_OK ||
        esp_lcd_panel_init(panel) != ESP_OK)
        return false;
    if (esp_lcd_panel_swap_xy(panel, p.swap) != ESP_OK ||
        esp_lcd_panel_mirror(panel, p.mirrorX, p.mirrorY) != ESP_OK ||
        esp_lcd_panel_invert_color(panel, p.invert) != ESP_OK)
        return false;
    if (esp_lcd_panel_disp_on_off(panel, true) != ESP_OK)
        return false;
    if (!XTouchPort::prepareBacklight())
        return false;
    return true;
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    if (pending) {
        if (xSemaphoreTake(complete, 0) != pdTRUE)
            return;
        pending = false;
        lv_obj_invalidate(lv_scr_act());
    }
    const auto &p = XTouchPort::profile();
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > p.width || y + h > p.height)
        return;
    for (int row = 0; row < h; row += Rows) {
        int n = std::min<int>(Rows, h - row);
        for (int k = 0; k < w * n; ++k) {
            uint16_t v = pixels[row * w + k];
            tile[k] = uint16_t(v << 8 | v >> 8);
        }
        if (esp_lcd_panel_draw_bitmap(panel, x, y + row, x + w, y + row + n, tile) != ESP_OK)
            return;
        pending = true;
        if (xSemaphoreTake(complete, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGE("display", "SPI completion timeout");
            return; // Do not overwrite the DMA tile or reconnect the printer on a display fault.
        }
        pending = false;
    }
}
void board_flush() {
    if (pending && xSemaphoreTake(complete, 0) == pdTRUE) {
        pending = false;
        lv_obj_invalidate(lv_scr_act());
    }
}
void board_invert(bool value) {
    esp_lcd_panel_invert_color(panel, XTouchPort::profile().invert ^ value);
}
bool board_display_sleep(bool sleeping) {
    if (sleeping && esp_lcd_panel_disp_on_off(panel, false) != ESP_OK)
        return false;
    if (esp_lcd_panel_io_tx_param(io, sleeping ? 0x10 : 0x11, nullptr, 0) != ESP_OK)
        return false;
    delay(120);
    return sleeping || esp_lcd_panel_disp_on_off(panel, true) == ESP_OK;
}
