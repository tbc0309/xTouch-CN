#include "board.h"
#include "geometry.h"
#include "st7789_init.h"
#include "portrait_frame.h"
#include "platform/idf/i2c.h"
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <freertos/semphr.h>
static esp_lcd_panel_handle_t panel;
static esp_lcd_panel_io_handle_t panelIO;
static constexpr unsigned DrawRows = 48;
static uint16_t *drawPixels;
static SemaphoreHandle_t transferDone;
static uint16_t *framePixels, *transferPixels;
static int dirtyFirst = 320, dirtyEnd = 0, dirtyLeft = 240, dirtyRight = 0;
static bool transferPending;
static void present(bool wait);
static void waitTransfer() {
    if (transferPending) {
        xSemaphoreTake(transferDone, portMAX_DELAY);
        transferPending = false;
    }
}
static bool colorDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(transferDone, &wake);
    return wake == pdTRUE;
}
static bool regRead(uint32_t reg, uint8_t *data, size_t n) {
    uint8_t address[] = {uint8_t(reg >> 24), uint8_t(reg >> 16), uint8_t(reg >> 8), uint8_t(reg)};
    return xtouch_i2c_aux.transfer(0x58, address, sizeof(address), data, n) == ESP_OK;
}
static void touchAck() {
    const uint8_t ack[] = {0xd0, 0, 2, 0xab};
    xtouch_i2c_aux.transfer(0x58, ack, sizeof(ack), nullptr, 0);
}
bool board_display_begin() {
    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);
    pinMode(6, INPUT_PULLUP);
    spi_bus_config_t bus = {};
    bus.mosi_io_num = 45;
    bus.miso_io_num = -1;
    bus.sclk_io_num = 40;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 320 * 240 * 2;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    transferDone = xSemaphoreCreateBinary();
    if (!transferDone)
        return false;
    esp_lcd_panel_io_spi_config_t io = {};
    io.dc_gpio_num = GPIO_NUM_41;
    io.cs_gpio_num = GPIO_NUM_42;
    io.pclk_hz = 80000000;
    io.lcd_cmd_bits = 8;
    io.lcd_param_bits = 8;
    io.trans_queue_depth = 2;
    io.on_color_trans_done = colorDone;
    io.flags.psram_dma_direct = 1;
    if (esp_lcd_new_panel_io_spi(SPI2_HOST, &io, &panelIO) != ESP_OK)
        return false;
    esp_lcd_panel_dev_config_t dev = {};
    dev.reset_gpio_num = GPIO_NUM_39;
    dev.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    dev.bits_per_pixel = 16;
    dev.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE;
    if (esp_lcd_new_panel_st7789(panelIO, &dev, &panel) != ESP_OK)
        return false;
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    for (const auto &c : s3_panel_init)
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(panelIO, c.command, c.data, c.length));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    // Compose in PSRAM before updating panel GRAM, avoiding render gaps between
    // stripes. The board exposes no TE input; this is not VSYNC synchronization.
    framePixels =
        (uint16_t *)heap_caps_aligned_alloc(64, 320 * 240 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    transferPixels =
        (uint16_t *)heap_caps_aligned_alloc(64, 320 * 240 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!framePixels || !transferPixels)
        return false;
    memset(framePixels, 0, 320 * 240 * 2);
    drawPixels =
        (uint16_t *)heap_caps_malloc(320 * DrawRows * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!drawPixels)
        return false;
    ledcAttach(5, 20000, 10);
    if (!xtouch_i2c_aux.begin(1, 3, 400000))
        return false;
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    delay(100);
    digitalWrite(2, HIGH);
    delay(500);
    pinMode(4, INPUT);
    return true;
}
bool board_draw_buffers(uint16_t **first, uint16_t **second, uint32_t *pixels) {
    *first = (uint16_t *)drawPixels;
    *second = nullptr;
    *pixels = 320 * DrawRows;
    return true;
}
bool board_pixels_async(lv_display_t *driver, int x, int y, int w, int h, const uint16_t *p) {
    board_pixels(x, y, w, h, p);
    if (lv_disp_flush_is_last(driver))
        present(false);
    lv_disp_flush_ready(driver);
    return true;
}
void board_pixels(int x, int y, int w, int h, const uint16_t *p) {
    if (w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > 320 || y + h > 240)
        return;
    // Landscape (x,y) maps to native portrait (239-y,x), as does touch.
    // Rotate during composition so SPI writes follow the panel's native rows.
    PortraitFrame::copy(framePixels, 240, x, y, w, h, p);
    if (240 - y - h < dirtyLeft)
        dirtyLeft = 240 - y - h;
    if (240 - y > dirtyRight)
        dirtyRight = 240 - y;
    if (x < dirtyFirst)
        dirtyFirst = x;
    if (x + w > dirtyEnd)
        dirtyEnd = x + w;
}
void board_brightness(uint8_t v) {
    ledcWrite(5, (uint32_t)v * 1023 / 255);
}
void board_invert(bool v) {
    esp_lcd_panel_invert_color(panel, !v);
}
bool board_touch(int &x, int &y) {
    uint8_t p[9] = {};
    if (!regRead(0xd0070000, p, 9))
        return false;
    touchAck();
    if (!(p[3] & 15) || !(p[8] & 0xf0))
        return false;
    unsigned nx = ((p[7] & 15) << 8) | p[4], ny = ((p[7] & 0xf0) << 4) | p[5];
    BoardGeometry::Point point;
    if (!BoardGeometry::s3Touch(nx, ny, point))
        return false;
    x = point.x;
    y = point.y;
    return true;
}
static void present(bool wait) {
    if (dirtyEnd <= dirtyFirst) {
        if (wait)
            waitTransfer();
        return;
    }
    // DMA reads only transferPixels; the next LVGL frame may compose into
    // framePixels concurrently. Never rewrite the outgoing buffer before completion.
    waitTransfer();
    // Align packed rows for PSRAM DMA without per-frame allocation. Small label
    // updates retain a narrow transfer instead of resending the full screen width.
    // Sixteen native columns and two rows give a 64-byte DMA-aligned length.
    int left = dirtyLeft & ~15, right = (dirtyRight + 15) & ~15, w = right - left;
    int top = dirtyFirst & ~1, bottom = (dirtyEnd + 1) & ~1;
    for (int y = top; y < bottom; ++y)
        memcpy(transferPixels + (y - top) * w, framePixels + y * 240 + left, w * 2);
    // One uninterrupted SPI DMA burst instead of render/transfer/render gaps.
    transferPending = true;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, left, top, right, bottom, transferPixels));
    dirtyFirst = 320;
    dirtyEnd = 0;
    dirtyLeft = 240;
    dirtyRight = 0;
    if (wait)
        waitTransfer();
}
void board_flush() {
    present(true);
}

// DCS display off + sleep in saves panel power; touch controller stays awake.
bool board_display_sleep(bool sleeping) {
    if (!panelIO)
        return false;
    board_flush();
    if (sleeping) {
        if (esp_lcd_panel_io_tx_param(panelIO, 0x28, nullptr, 0) != ESP_OK)
            return false;
        if (esp_lcd_panel_io_tx_param(panelIO, 0x10, nullptr, 0) != ESP_OK)
            return false;
        delay(120);
    } else {
        if (esp_lcd_panel_io_tx_param(panelIO, 0x11, nullptr, 0) != ESP_OK)
            return false;
        delay(120);
        if (esp_lcd_panel_io_tx_param(panelIO, 0x29, nullptr, 0) != ESP_OK)
            return false;
    }
    return true;
}
