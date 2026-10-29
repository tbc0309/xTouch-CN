#include "board.h"
#include "bsp_touch_packets.h"
#include "touch_contact.h"
#include <algorithm>
#include <bsp/display.h>
#include <bsp/esp-bsp.h>
#include <bsp/touch.h>
#include <driver/i2c_master.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <freertos/semphr.h>

// The BSP owns peripheral creation only. The application's existing LVGL task
// remains the sole UI owner; no bsp_display_start()/second LVGL instance is used.
namespace {
constexpr unsigned Rows = 16;
esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t io;
esp_lcd_touch_handle_t touch;
SemaphoreHandle_t completed;
uint16_t *tile;
bool pending;
bool tt21100;
TouchContact contact;

bool IRAM_ATTR transferDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(completed, &wake);
    return wake == pdTRUE;
}
} // namespace

bool board_display_begin() {
    static_assert(BSP_LCD_BITS_PER_PIXEL == 16, "BSP transport requires RGB565");
    static_assert(BSP_LCD_H_RES == XTOUCH_DISPLAY_WIDTH && BSP_LCD_V_RES == XTOUCH_DISPLAY_HEIGHT,
                  "BSP screen selection mismatch");
    completed = xSemaphoreCreateBinary();
    tile = static_cast<uint16_t *>(
        heap_caps_malloc(BSP_LCD_H_RES * Rows * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (!completed || !tile)
        return false;
    bsp_display_config_t config{};
    config.max_transfer_sz = BSP_LCD_H_RES * Rows * 2;
    if (bsp_display_new(&config, &panel, &io) != ESP_OK)
        return false;
    esp_lcd_panel_io_callbacks_t callbacks{};
    callbacks.on_color_trans_done = transferDone;
    if (esp_lcd_panel_io_register_event_callbacks(io, &callbacks, nullptr) != ESP_OK ||
        esp_lcd_panel_disp_on_off(panel, true) != ESP_OK ||
        bsp_touch_new(nullptr, &touch) != ESP_OK)
        return false;
    const auto bus = bsp_i2c_get_handle();
    tt21100 = i2c_master_probe(bus, 0x5d, 100) != ESP_OK &&
              i2c_master_probe(bus, 0x14, 100) != ESP_OK &&
              i2c_master_probe(bus, 0x24, 100) == ESP_OK;
    return true;
}

void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    if (!panel || !pixels || x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > BSP_LCD_H_RES ||
        y + h > BSP_LCD_V_RES)
        return;
    if (pending) {
        if (xSemaphoreTake(completed, 0) != pdTRUE)
            return;
        pending = false;
        lv_obj_invalidate(lv_scr_act());
    }
    for (int row = 0; row < h; row += Rows) {
        const int count = std::min<int>(Rows, h - row);
        for (int i = 0; i < count * w; ++i) {
            const uint16_t value = pixels[row * w + i];
            tile[i] = BSP_LCD_BIGENDIAN ? uint16_t((value << 8) | (value >> 8)) : value;
        }
        xSemaphoreTake(completed, 0);
        if (esp_lcd_panel_draw_bitmap(panel, x, y + row, x + w, y + row + count, tile) != ESP_OK)
            return;
        pending = true;
        if (xSemaphoreTake(completed, pdMS_TO_TICKS(100)) != pdTRUE)
            return; // Keep DMA storage intact until its completion callback.
        pending = false;
    }
}

void board_flush() {}

bool board_touch(int &x, int &y) {
    if (!touch)
        return contact.read(x, y);
    if (tt21100) {
        // Peeking at length does not consume a report: the controller's own
        // driver uses the same length-read followed by full-report-read sequence.
        uint8_t lengthBytes[2]{};
        if (esp_lcd_panel_io_rx_param(touch->io, -1, lengthBytes, 2) != ESP_OK)
            return contact.read(x, y);
        const unsigned length = lengthBytes[0] | static_cast<unsigned>(lengthBytes[1]) << 8;
        if (length == 0 || length == 2)
            return contact.read(x, y);
        uint8_t packet[57]{};
        if (length > sizeof(packet) || (length != 14 && (length < 7 || (length - 7) % 10)))
            return contact.read(x, y);
        if (esp_lcd_panel_io_rx_param(touch->io, -1, packet, length) != ESP_OK)
            return contact.read(x, y);
        int tx = 0, ty = 0;
        const auto event = CandidateTouch::tt21100(packet, length, tx, ty);
        if (touch->config.flags.swap_xy)
            std::swap(tx, ty);
        if (touch->config.flags.mirror_x)
            tx = touch->config.x_max - 1 - tx;
        if (touch->config.flags.mirror_y)
            ty = touch->config.y_max - 1 - ty;
        if (event == CandidateTouch::Event::Ignore ||
            (event == CandidateTouch::Event::Pressed &&
             (tx < 0 || ty < 0 || tx >= BSP_LCD_H_RES || ty >= BSP_LCD_V_RES)))
            return contact.read(x, y);
        return contact.report(event == CandidateTouch::Event::Pressed, tx, ty, x, y);
    } else {
        uint8_t state = 0;
        if (esp_lcd_panel_io_rx_param(touch->io, 0x814e, &state, 1) != ESP_OK ||
            (state & 0x80) == 0)
            return contact.read(x, y);
        if ((state & 15) > 5) {
            const uint8_t clear = 0;
            esp_lcd_panel_io_tx_param(touch->io, 0x814e, &clear, 1);
            return contact.read(x, y);
        }
    }
    if (esp_lcd_touch_read_data(touch) != ESP_OK)
        return contact.read(x, y);
    esp_lcd_touch_point_data_t point{};
    uint8_t points = 0;
    if (esp_lcd_touch_get_data(touch, &point, &points, 1) != ESP_OK)
        return contact.read(x, y);
    const bool pressed = points > 0;
    if (pressed && (point.x >= BSP_LCD_H_RES || point.y >= BSP_LCD_V_RES))
        return contact.read(x, y);
    return contact.report(pressed, point.x, point.y, x, y);
}

void board_brightness(uint8_t level) { bsp_display_brightness_set(uint32_t(level) * 100 / 255); }

void board_invert(bool inverted) {
    if (panel)
        esp_lcd_panel_invert_color(panel, inverted);
}

bool board_display_sleep(bool sleeping) {
    if (!panel || !io)
        return false;
    if (sleeping && esp_lcd_panel_disp_on_off(panel, false) != ESP_OK)
        return false;
    if (esp_lcd_panel_io_tx_param(io, sleeping ? 0x10 : 0x11, nullptr, 0) != ESP_OK)
        return false;
    delay(120);
    if (!sleeping) {
        if (esp_lcd_panel_disp_on_off(panel, true) != ESP_OK)
            return false;
        lv_obj_invalidate(lv_scr_act());
    }
    return true;
}
