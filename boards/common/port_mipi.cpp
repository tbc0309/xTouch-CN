#include "board.h"
#include "port_profile.h"
#include "portrait_frame.h"
#include "port_video_config.h"
#include <esp_cache.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_ldo_regulator.h>
#include <esp_log.h>
#include <cstring>
#include <algorithm>

namespace {
esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t commands;
esp_ldo_channel_handle_t phyPower;
uint16_t *back, *front;
volatile uint32_t frames;
bool inverted, waiting, redraw;
int first, end;
uint32_t submitted;
bool IRAM_ATTR complete(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t *, void *) {
    ++frames;
    return false;
}
} // namespace
bool board_display_begin() {
    const auto &v = XTouchPort::videoProfile();
    if (!XTouchPort::preparePeripherals())
        return false;
    esp_ldo_channel_config_t power{};
    power.chan_id = 3;
    power.voltage_mv = 2500;
    if (esp_ldo_acquire_channel(&power, &phyPower) != ESP_OK)
        return false;
    esp_lcd_dsi_bus_config_t bus{};
    bus.bus_id = 0;
    bus.num_data_lanes = 2;
    bus.lane_bit_rate_mbps = v.laneMbps;
    esp_lcd_dsi_bus_handle_t dsi;
    if (esp_lcd_new_dsi_bus(&bus, &dsi) != ESP_OK)
        return false;
    esp_lcd_dbi_io_config_t dbi{};
    dbi.lcd_cmd_bits = 8;
    dbi.lcd_param_bits = 8;
    if (esp_lcd_new_panel_io_dbi(dsi, &dbi, &commands) != ESP_OK ||
        portCreatePanel(dsi, commands, &panel) != ESP_OK || esp_lcd_panel_reset(panel) != ESP_OK ||
        esp_lcd_panel_init(panel) != ESP_OK)
        return false;
    // Factory config owns the module timing and orientation; UI tiles use landscape coordinates.
    void *buffers[2]{};
    if (esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &buffers[0], &buffers[1]) != ESP_OK)
        return false;
    front = static_cast<uint16_t *>(buffers[0]);
    back = static_cast<uint16_t *>(buffers[1]);
    std::memset(front, 0, size_t(v.width) * v.height * 2);
    std::memset(back, 0, size_t(v.width) * v.height * 2);
    esp_lcd_dpi_panel_event_callbacks_t callbacks{};
    callbacks.on_frame_buf_complete = complete;
    if (esp_lcd_dpi_panel_register_event_callbacks(panel, &callbacks, nullptr) != ESP_OK)
        return false;
    first = v.height;
    return XTouchPort::prepareBacklight();
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    const auto &p = XTouchPort::profile();
    const auto &v = XTouchPort::videoProfile();
    if (waiting) {
        redraw = true;
        return;
    }
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > p.width || y + h > p.height)
        return;
    if (v.portrait)
        PortraitFrame::copy(back, v.width, x, y, w, h, pixels, inverted ? 0xffff : 0);
    else
        for (int row = 0; row < h; ++row)
            for (int col = 0; col < w; ++col)
                back[(p.mirrorY ? v.height - 1 - y - row : y + row) * v.width +
                     (p.mirrorX ? v.width - 1 - x - col : x + col)] =
                    pixels[row * w + col] ^ (inverted ? 0xffff : 0);
    const int beginRow = v.portrait ? x : p.mirrorY ? v.height - y - h : y;
    first = std::min(first, beginRow);
    end = std::max(end, beginRow + (v.portrait ? w : h));
}
void board_flush() {
    const auto &v = XTouchPort::videoProfile();
    if (end <= first)
        return;
    if (!waiting) {
        esp_cache_msync(back + size_t(first) * v.width, size_t(end - first) * v.width * 2,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
        submitted = frames;
        if (esp_lcd_panel_draw_bitmap(panel, 0, first, v.width, end, back) != ESP_OK)
            return;
        waiting = true;
    }
    const uint32_t start = millis();
    while (uint32_t(frames - submitted) < 2 && uint32_t(millis() - start) < 100)
        vTaskDelay(1);
    if (uint32_t(frames - submitted) < 2) {
        ESP_LOGW("display", "DSI frame wait timeout");
        return;
    }
    waiting = false;
    std::swap(front, back);
    const size_t offset = size_t(first) * v.width, bytes = size_t(end - first) * v.width * 2;
    std::memcpy(back + offset, front + offset, bytes);
    esp_cache_msync(back + offset, bytes,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    first = v.height;
    end = 0;
    if (redraw) {
        redraw = false;
        lv_obj_invalidate(lv_scr_act());
    }
}
void board_invert(bool value) {
    inverted = value;
    lv_obj_invalidate(lv_scr_act());
}
bool board_display_sleep(bool sleeping) {
    board_flush();
    if (sleeping && esp_lcd_panel_io_tx_param(commands, 0x28, nullptr, 0) != ESP_OK)
        return false;
    if (esp_lcd_panel_io_tx_param(commands, sleeping ? 0x10 : 0x11, nullptr, 0) != ESP_OK)
        return false;
    delay(120);
    if (!sleeping) {
        if (esp_lcd_panel_io_tx_param(commands, 0x29, nullptr, 0) != ESP_OK)
            return false;
        lv_obj_invalidate(lv_scr_act());
    }
    return true;
}
