#include "board.h"
#include "port_profile.h"
#include "portrait_frame.h"
#include "panel_init.h"
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_axs15231b.h>
#include <esp_log.h>
#include <freertos/semphr.h>
#include <algorithm>
#include <cstring>
namespace {
esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t io;
SemaphoreHandle_t complete;
uint16_t *frame, *tile;
bool dirty, inverted, pending;
esp_err_t command(uint8_t value) {
    return esp_lcd_panel_io_tx_param(io, 0x02000000U | (uint32_t(value) << 8), nullptr, 0);
}
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
    frame = static_cast<uint16_t *>(
        heap_caps_calloc(size_t(p.width) * p.height, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    tile = static_cast<uint16_t *>(
        heap_caps_malloc(p.height * 16 * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    complete = xSemaphoreCreateBinary();
    if (!frame || !tile || !complete)
        return false;
    spi_bus_config_t bus{};
    bus.sclk_io_num = p.clock;
    bus.mosi_io_num = p.mosi;
    bus.miso_io_num = p.miso;
    bus.quadwp_io_num = p.data2;
    bus.quadhd_io_num = p.data3;
    bus.max_transfer_sz = p.height * 16 * 2;
    if (spi_bus_initialize(spi_host_device_t(p.spiHost), &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    esp_lcd_panel_io_spi_config_t cfg{};
    cfg.cs_gpio_num = gpio_num_t(p.cs);
    cfg.dc_gpio_num = GPIO_NUM_NC;
    cfg.pclk_hz = p.hz;
    cfg.spi_mode = 3;
    cfg.lcd_cmd_bits = 32;
    cfg.lcd_param_bits = 8;
    cfg.trans_queue_depth = 1;
    cfg.flags.quad_mode = true;
    cfg.on_color_trans_done = done;
    if (esp_lcd_new_panel_io_spi(spi_host_device_t(p.spiHost), &cfg, &io) != ESP_OK)
        return false;
    axs15231b_vendor_config_t vendor{};
    vendor.init_cmds = lcd_init_cmds;
    vendor.init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]);
    vendor.flags.use_qspi_interface = 1;
    esp_lcd_panel_dev_config_t dev{};
    dev.reset_gpio_num = GPIO_NUM_NC;
    dev.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    dev.bits_per_pixel = 16;
    dev.vendor_config = &vendor;
    if (esp_lcd_new_panel_axs15231b(io, &dev, &panel) != ESP_OK ||
        esp_lcd_panel_reset(panel) != ESP_OK || esp_lcd_panel_init(panel) != ESP_OK ||
        command(0x29) != ESP_OK)
        return false;
    return XTouchPort::prepareBacklight();
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    const auto &p = XTouchPort::profile();
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > p.width || y + h > p.height)
        return;
    PortraitFrame::copy(frame, p.height, x, y, w, h, pixels, inverted ? 0xffff : 0,
                        PortraitFrame::ByteOrder::Swap16);
    dirty = true;
}
void board_flush() {
    if (pending) {
        if (xSemaphoreTake(complete, 0) != pdTRUE)
            return;
        pending = false;
    }
    if (!dirty)
        return;
    const auto &p = XTouchPort::profile();
    // AXS QSPI omits RASET. Start each complete frame with RAMWR, then RAMWRC stripes.
    for (int row = 0; row < p.width; row += 16) {
        const int n = std::min(16, p.width - row);
        std::memcpy(tile, frame + row * p.height, size_t(n) * p.height * 2);
        if (esp_lcd_panel_draw_bitmap(panel, 0, row, p.height, row + n, tile) != ESP_OK)
            return;
        pending = true;
        if (xSemaphoreTake(complete, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGE("display", "QSPI completion timeout");
            return;
        }
        pending = false;
    }
    dirty = false;
}
void board_invert(bool value) {
    inverted = value;
    lv_obj_invalidate(lv_scr_act());
}
bool board_display_sleep(bool sleeping) {
    board_flush();
    // The pinned AXS driver's legacy disp_off callback reverses IDF 6's on/off semantics.
    // Send the documented QSPI DCS commands explicitly instead of inverting a Boolean workaround.
    if (sleeping && command(0x28) != ESP_OK)
        return false;
    if (command(sleeping ? 0x10 : 0x11) != ESP_OK)
        return false;
    delay(120);
    if (!sleeping) {
        if (command(0x29) != ESP_OK)
            return false;
        lv_obj_invalidate(lv_scr_act());
    }
    return true;
}
