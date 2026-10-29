#pragma once
#include "board.h"
#include <algorithm>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <freertos/semphr.h>
class CandidateSpiTransport {
    static constexpr unsigned Rows = 16;
    esp_lcd_panel_handle_t panel = nullptr;
    esp_lcd_panel_io_handle_t io = nullptr;
    SemaphoreHandle_t done = nullptr;
    uint16_t *tile = nullptr;
    bool pending = false;
    static bool IRAM_ATTR completed(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *,
                                    void *ctx) {
        auto *self = static_cast<CandidateSpiTransport *>(ctx);
        BaseType_t wake = pdFALSE;
        xSemaphoreGiveFromISR(self->done, &wake);
        return wake == pdTRUE;
    }

  public:
    bool begin(esp_lcd_panel_handle_t lcd, esp_lcd_panel_io_handle_t commands) {
        if (!lcd || !commands || panel)
            return false;
        tile = static_cast<uint16_t *>(heap_caps_malloc(XTOUCH_DISPLAY_WIDTH * Rows * 2,
                                                        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
        done = xSemaphoreCreateBinary();
        if (!tile || !done) {
            if (tile)
                heap_caps_free(tile);
            if (done)
                vSemaphoreDelete(done);
            tile = nullptr;
            done = nullptr;
            return false;
        }
        esp_lcd_panel_io_callbacks_t events{};
        events.on_color_trans_done = completed;
        if (esp_lcd_panel_io_register_event_callbacks(commands, &events, this) != ESP_OK) {
            heap_caps_free(tile);
            vSemaphoreDelete(done);
            tile = nullptr;
            done = nullptr;
            return false;
        }
        panel = lcd;
        io = commands;
        return true;
    }
    void draw(int x, int y, int w, int h, const uint16_t *pixels) {
        if (!panel || !pixels || x < 0 || y < 0 || w <= 0 || h <= 0 ||
            x + w > XTOUCH_DISPLAY_WIDTH || y + h > XTOUCH_DISPLAY_HEIGHT)
            return;
        if (pending) {
            if (xSemaphoreTake(done, 0) != pdTRUE)
                return;
            pending = false;
            lv_obj_invalidate(lv_scr_act());
        }
        for (int row = 0; row < h; row += Rows) {
            const int count = std::min<int>(Rows, h - row);
            for (int i = 0; i < w * count; ++i) {
                const auto v = pixels[row * w + i];
                tile[i] = uint16_t(v << 8 | v >> 8);
            }
            if (esp_lcd_panel_draw_bitmap(panel, x, y + row, x + w, y + row + count, tile) !=
                ESP_OK)
                return;
            pending = true;
            if (xSemaphoreTake(done, pdMS_TO_TICKS(1000)) != pdTRUE)
                return; // A timeout never gives permission to overwrite in-flight DMA memory.
            pending = false;
        }
    }
    void flush() {
        if (pending && xSemaphoreTake(done, 0) == pdTRUE) {
            pending = false;
            lv_obj_invalidate(lv_scr_act());
        }
    }
    bool sleep(bool sleeping) {
        if (!panel || (sleeping && esp_lcd_panel_disp_on_off(panel, false) != ESP_OK) ||
            esp_lcd_panel_io_tx_param(io, sleeping ? 0x10 : 0x11, nullptr, 0) != ESP_OK)
            return false;
        delay(120);
        return sleeping || esp_lcd_panel_disp_on_off(panel, true) == ESP_OK;
    }
    void invert(bool value) {
        if (panel)
            esp_lcd_panel_invert_color(panel, value);
    }
};
