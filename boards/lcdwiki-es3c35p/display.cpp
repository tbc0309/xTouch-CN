#include "board.h"
#include "geometry.h"
#include "portrait_frame.h"
#include "../common/touch_contact.h"
#include "st77922_init.h"
#include "platform/idf/i2c.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <freertos/semphr.h>
#include <cstring>
#include <algorithm>
namespace {
constexpr int Width = 480, Height = 320, NativeWidth = 320, NativeHeight = 480;
// Composition targets PSRAM frame buffers; this tile is not the SPI queue.
// Keep internal DMA RAM available for networking and the camera worker.
constexpr unsigned DrawRows = 16;
constexpr uint8_t TouchAddress = 0x55;
constexpr const char *Tag = "es3c35p";
// This glass needs INVON for normal colors (also present in its vendor init).
// The user inversion preference is relative to this electrical polarity.
constexpr bool NormalColorInversion = true;
esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t panelIO;
SemaphoreHandle_t transferDone;
uint16_t *drawPixels, *framePixels, *transferPixels;
bool transferPending = false, touchReady = false;
uint8_t touchPoints = 1;
TouchContact touchContact;
int dirtyFirst = NativeHeight, dirtyEnd = 0, dirtyLeft = NativeWidth, dirtyRight = 0;
void waitTransfer() {
    if (transferPending) {
        xSemaphoreTake(transferDone, portMAX_DELAY);
        transferPending = false;
    }
}
bool colorDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(transferDone, &wake);
    return wake == pdTRUE;
}
bool readTouch(uint16_t reg, uint8_t *out, size_t bytes) {
    const uint8_t address[] = {uint8_t(reg >> 8), uint8_t(reg)};
    return xtouch_i2c.transfer(TouchAddress, address, sizeof(address), out, bytes) == ESP_OK;
}
void present(bool wait) {
    if (dirtyEnd <= dirtyFirst) {
        if (wait)
            waitTransfer();
        return;
    }
    waitTransfer();
    // Pack aligned native rows into a separate outgoing buffer. The next LVGL
    // frame may compose concurrently, but cannot overwrite queued DMA data.
    int left = dirtyLeft & ~15, right = (dirtyRight + 15) & ~15, w = right - left;
    int top = dirtyFirst & ~1, bottom = (dirtyEnd + 1) & ~1;
    for (int row = top; row < bottom; ++row)
        std::memcpy(transferPixels + (row - top) * w, framePixels + row * NativeWidth + left,
                    w * 2);
    transferPending = true;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, left, top, right, bottom, transferPixels));
    dirtyFirst = NativeHeight;
    dirtyEnd = 0;
    dirtyLeft = NativeWidth;
    dirtyRight = 0;
    if (wait)
        waitTransfer();
}
} // namespace
bool board_display_begin() {
    gpio_config_t outputs{};
    outputs.pin_bit_mask = (1ULL << 1) | (1ULL << 41) | (1ULL << 48);
    outputs.mode = GPIO_MODE_OUTPUT;
    if (gpio_config(&outputs) != ESP_OK)
        return false;
    gpio_set_level(GPIO_NUM_1, 1);
    gpio_set_level(GPIO_NUM_41, 0);
    gpio_set_level(GPIO_NUM_48, 1);
    // QSPI40 needs more bandwidth than direct PSRAM DMA sustained during boot.
    // Let IDF stage at most two 5 KiB chunks in internal DMA RAM.
    spi_bus_config_t bus{};
    bus.sclk_io_num = 12;
    bus.data0_io_num = 11;
    bus.data1_io_num = 13;
    bus.data2_io_num = 14;
    bus.data3_io_num = 9;
    bus.max_transfer_sz = NativeWidth * 8 * 2;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    transferDone = xSemaphoreCreateBinary();
    if (!transferDone)
        return false;
    esp_lcd_panel_io_spi_config_t io{};
    io.cs_gpio_num = GPIO_NUM_10;
    io.dc_gpio_num = GPIO_NUM_NC;
    io.spi_mode = 0;
    io.pclk_hz = 40000000;
    io.trans_queue_depth = 2;
    io.lcd_cmd_bits = 32;
    io.lcd_param_bits = 8;
    io.flags.quad_mode = true;
    io.flags.psram_dma_direct = false;
    io.on_color_trans_done = colorDone;
    if (esp_lcd_new_panel_io_spi(SPI2_HOST, &io, &panelIO) != ESP_OK)
        return false;
    st77922_vendor_config_t vendor{};
    vendor.init_cmds = es3c35p_panel_init;
    vendor.init_cmds_size = sizeof(es3c35p_panel_init) / sizeof(es3c35p_panel_init[0]);
    vendor.flags.use_qspi_interface = 1;
    esp_lcd_panel_dev_config_t config{};
    config.reset_gpio_num = GPIO_NUM_NC;
    config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    config.bits_per_pixel = 16;
    config.vendor_config = &vendor;
    if (esp_lcd_new_panel_st77922(panelIO, &config, &panel) != ESP_OK)
        return false;
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, NormalColorInversion));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    // This glass ignores MADCTL.MV. Rotate during composition, not in the panel.
    framePixels = static_cast<uint16_t *>(
        heap_caps_aligned_alloc(64, Width * Height * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    transferPixels = static_cast<uint16_t *>(
        heap_caps_aligned_alloc(64, Width * Height * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    drawPixels = static_cast<uint16_t *>(
        heap_caps_malloc(Width * DrawRows * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!framePixels || !transferPixels || !drawPixels)
        return false;
    std::memset(framePixels, 0, Width * Height * 2);
    ledc_timer_config_t timer{};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = 20000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer) != ESP_OK)
        return false;
    ledc_channel_config_t light{};
    light.gpio_num = 41;
    light.speed_mode = LEDC_LOW_SPEED_MODE;
    light.channel = LEDC_CHANNEL_0;
    light.timer_sel = LEDC_TIMER_0;
    if (ledc_channel_config(&light) != ESP_OK)
        return false;
    if (!xtouch_i2c.begin(38, 39, 400000))
        return false;
    gpio_set_level(GPIO_NUM_48, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(GPIO_NUM_48, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    touchReady = xtouch_i2c.probe(TouchAddress) == ESP_OK && readTouch(0x0009, &touchPoints, 1) &&
                 touchPoints >= 1 && touchPoints <= 5;
    ESP_LOGI(Tag,
             "ST77922 QSPI40, landscape480x320, large resources; touch55=%s points=%u codec18=%s",
             touchReady ? "ready" : "unavailable", unsigned(touchPoints),
             xtouch_i2c.probe(0x18) == ESP_OK ? "present" : "absent");
    return true;
}
bool board_draw_buffers(uint16_t **first, uint16_t **second, uint32_t *pixels) {
    *first = drawPixels;
    *second = nullptr;
    *pixels = Width * DrawRows;
    return true;
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    if (!pixels || w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > Width || y + h > Height)
        return;
    // RGB565 is native little-endian in LVGL; this QSPI panel needs swapped bytes.
    PortraitFrame::copy(framePixels, NativeWidth, x, y, w, h, pixels, 0,
                        PortraitFrame::ByteOrder::Swap16);
    dirtyLeft = std::min(dirtyLeft, NativeWidth - y - h);
    dirtyRight = std::max(dirtyRight, NativeWidth - y);
    dirtyFirst = std::min(dirtyFirst, x);
    dirtyEnd = std::max(dirtyEnd, x + w);
}
bool board_pixels_async(lv_display_t *display, int x, int y, int w, int h, const uint16_t *pixels) {
    board_pixels(x, y, w, h, pixels);
    if (lv_disp_flush_is_last(display))
        present(false);
    lv_disp_flush_ready(display);
    return true;
}
void board_flush() {
    present(true);
}
void board_brightness(uint8_t value) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, uint32_t(value) * 1023 / 255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
void board_invert(bool value) {
    board_flush();
    esp_lcd_panel_invert_color(panel, NormalColorInversion != value);
}
bool board_display_sleep(bool sleeping) {
    if (!panelIO)
        return false;
    board_flush();
    // LCD and touch share this controller. SLPIN also stops its touch reports,
    // so online standby must leave the controller running with its display off.
    // The common layer already turns the backlight off and pauses rendering.
    return esp_lcd_panel_disp_on_off(panel, !sleeping) == ESP_OK;
}
bool board_touch(int &x, int &y) {
    if (!touchReady)
        return false;
    uint8_t info = 0;
    if (!readTouch(0x0010, &info, 1) || !(info & 0x08))
        return touchContact.read(x, y);
    uint8_t points[7 * 5]{};
    if (!readTouch(0x0014, points, 7 * touchPoints))
        return touchContact.read(x, y);
    // Read every supported point to acknowledge the controller's packet.
    for (unsigned n = 0; n < touchPoints; ++n) {
        const uint8_t *p = points + 7 * n;
        if (!(p[0] & 0x80))
            continue;
        int nx = ((p[0] & 0x3f) << 8) | p[1], ny = ((p[2] & 0x3f) << 8) | p[3];
        BoardGeometry::Point point;
        if (!BoardGeometry::portraitTouch(NativeWidth, NativeHeight, nx, ny, point))
            continue;
        return touchContact.report(true, point.x, point.y, x, y);
    }
    return touchContact.report(false, 0, 0, x, y);
}
