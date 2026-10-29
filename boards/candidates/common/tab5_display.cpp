#include "board.h"
#include "portrait_frame.h"
#include "touch_contact.h"
#include <algorithm>
#include <bsp/display.h>
#include <bsp/esp-bsp.h>
#include <bsp/touch.h>
#include <cstring>
#include <esp_cache.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_touch_st7123.h>
namespace {
esp_lcd_panel_handle_t panel;
esp_lcd_touch_handle_t touch;
uint16_t *front, *back;
volatile uint32_t frames;
uint32_t submitted;
bool waiting, inverted, dirty, redraw;
TouchContact contact;
bool gt911;
bool IRAM_ATTR completed(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t *, void *) {
    ++frames;
    return false;
}
} // namespace
bool board_display_begin() {
    static_assert(XTOUCH_DISPLAY_WIDTH == BSP_LCD_V_RES && XTOUCH_DISPLAY_HEIGHT == BSP_LCD_H_RES,
                  "Tab5 landscape canvas does not match native portrait panel");
    if (bsp_i2c_init() != ESP_OK || bsp_feature_enable(BSP_FEATURE_TOUCH, true) != ESP_OK)
        return false;
    delay(500);
    const auto bus = bsp_i2c_get_handle();
    bsp_display_config_t cfg{};
    cfg.dsi_bus.lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS;
    if (i2c_master_probe(bus, ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS, 100) == ESP_OK) {
        esp_lcd_panel_io_handle_t probe;
        esp_lcd_panel_io_i2c_config_t io{};
        io.scl_speed_hz = 100000;
        io.dev_addr = ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS;
        io.control_phase_bytes = 1;
        io.lcd_cmd_bits = 16;
        io.flags.disable_control_phase = 1;
        uint8_t version = 0;
        if (esp_lcd_new_panel_io_i2c(bus, &io, &probe) != ESP_OK)
            return false;
        const auto result = esp_lcd_panel_io_rx_param(probe, 0, &version, 1);
        esp_lcd_panel_io_del(probe);
        if (result != ESP_OK || (version != 1 && version != 3))
            return false;
        if (version == 1)
            cfg.dsi_bus.lane_bit_rate_mbps = 965;
    } else {
        if (i2c_master_probe(bus, 0x14, 100) != ESP_OK)
            return false; // Do not enter the BSP's assert for an unidentified panel.
        gt911 = true;
    }
    bsp_lcd_handles_t handles{};
    if (bsp_display_new_with_handles(&cfg, &handles) != ESP_OK)
        return false;
    panel = handles.panel;
    void *buffers[2]{};
    if (esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &buffers[0], &buffers[1]) != ESP_OK)
        return false;
    front = static_cast<uint16_t *>(buffers[0]);
    back = static_cast<uint16_t *>(buffers[1]);
    std::memset(front, 0, BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
    std::memset(back, 0, BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
    esp_lcd_dpi_panel_event_callbacks_t events{};
    events.on_frame_buf_complete = completed;
    return esp_lcd_dpi_panel_register_event_callbacks(panel, &events, nullptr) == ESP_OK &&
           bsp_touch_new(nullptr, &touch) == ESP_OK &&
           esp_lcd_panel_disp_on_off(panel, true) == ESP_OK;
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    if (waiting) {
        redraw = true;
        return;
    }
    if (!back || !pixels || x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > XTOUCH_DISPLAY_WIDTH ||
        y + h > XTOUCH_DISPLAY_HEIGHT)
        return;
    dirty = true;
    PortraitFrame::copy(back, BSP_LCD_H_RES, x, y, w, h, pixels, inverted ? 0xffff : 0);
}
void board_flush() {
    if (!panel || (!waiting && !dirty))
        return;
    if (!waiting) {
        if (esp_cache_msync(back, BSP_LCD_H_RES * BSP_LCD_V_RES * 2,
                            ESP_CACHE_MSYNC_FLAG_DIR_C2M) != ESP_OK)
            return;
        submitted = frames;
        if (esp_lcd_panel_draw_bitmap(panel, 0, 0, BSP_LCD_H_RES, BSP_LCD_V_RES, back) != ESP_OK)
            return;
        waiting = true;
    }
    const uint32_t started = millis();
    while (uint32_t(frames - submitted) < 2 && uint32_t(millis() - started) < 100)
        vTaskDelay(1);
    if (uint32_t(frames - submitted) < 2)
        return; // Preserve both buffers while DMA may still own them.
    std::swap(front, back);
    std::memcpy(back, front, BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
    waiting = false;
    dirty = false;
    if (redraw) {
        redraw = false;
        lv_obj_invalidate(lv_scr_act());
    }
}
bool board_touch(int &x, int &y) {
    if (touch && gt911) {
        uint8_t state = 0;
        if (esp_lcd_panel_io_rx_param(touch->io, 0x814e, &state, 1) != ESP_OK || (state & 0x80) == 0)
            return contact.read(x,y);
        if ((state & 15) > 5) {
            const uint8_t clear = 0;
            esp_lcd_panel_io_tx_param(touch->io, 0x814e, &clear, 1);
            return contact.read(x,y);
        }
    }
    if (!touch || esp_lcd_touch_read_data(touch) != ESP_OK)
        return contact.read(x, y);
    esp_lcd_touch_point_data_t point{};
    uint8_t count = 0;
    if (esp_lcd_touch_get_data(touch, &point, &count, 1) != ESP_OK)
        return contact.read(x, y);
    const bool down = count > 0;
    if (down && (point.x >= BSP_LCD_H_RES || point.y >= BSP_LCD_V_RES))
        return contact.read(x, y);
    return contact.report(down, point.y, BSP_LCD_H_RES - 1 - point.x, x, y);
}
void board_brightness(uint8_t value) {
    bsp_display_brightness_set(static_cast<unsigned>(value) * 100 / 255);
}
void board_invert(bool value) {
    inverted = value;
    lv_obj_invalidate(lv_scr_act());
}
bool board_display_sleep(bool sleeping) {
    if (!panel || esp_lcd_panel_disp_on_off(panel, !sleeping) != ESP_OK)
        return false;
    if (!sleeping)
        lv_obj_invalidate(lv_scr_act());
    return true;
}
