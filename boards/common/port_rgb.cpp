#include "board.h"
#include "port_profile.h"
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <esp_cache.h>
#include <esp_log.h>
#include <algorithm>
#include <cstring>
namespace {
esp_lcd_panel_handle_t panel;
uint16_t *back, *front;
volatile uint32_t frames;
uint32_t submitted;
bool waiting, redraw, inverted, asleep;
int first, end;
bool IRAM_ATTR complete(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *) {
    ++frames;
    return false;
}
} // namespace
bool board_display_begin() {
    const auto &p = XTouchPort::profile();
    const auto &r = XTouchPort::rgbProfile();
    if (!XTouchPort::preparePeripherals())
        return false;
    esp_lcd_rgb_panel_config_t cfg{};
    cfg.clk_src = LCD_CLK_SRC_DEFAULT;
    cfg.timings.pclk_hz = r.pixelClockHz;
    cfg.timings.h_res = p.width;
    cfg.timings.v_res = p.height;
    cfg.timings.hsync_pulse_width = r.hPulse;
    cfg.timings.hsync_back_porch = r.hBack;
    cfg.timings.hsync_front_porch = r.hFront;
    cfg.timings.vsync_pulse_width = r.vPulse;
    cfg.timings.vsync_back_porch = r.vBack;
    cfg.timings.vsync_front_porch = r.vFront;
    cfg.timings.flags.pclk_active_neg = r.negativeClock;
    cfg.data_width = 16;
    cfg.in_color_format = LCD_COLOR_FMT_RGB565;
    cfg.out_color_format = LCD_COLOR_FMT_RGB565;
    cfg.num_fbs = 2;
    cfg.bounce_buffer_size_px = p.width * 10;
    cfg.dma_burst_size = 64;
    cfg.hsync_gpio_num = gpio_num_t(r.hsync);
    cfg.vsync_gpio_num = gpio_num_t(r.vsync);
    cfg.de_gpio_num = gpio_num_t(r.de);
    cfg.pclk_gpio_num = gpio_num_t(r.clock);
    cfg.disp_gpio_num = gpio_num_t(r.displayEnable);
    for (unsigned i = 0; i < 16; ++i)
        cfg.data_gpio_nums[i] = gpio_num_t(r.data[i]);
    cfg.flags.fb_in_psram = 1;
    if (esp_lcd_new_rgb_panel(&cfg, &panel) != ESP_OK || esp_lcd_panel_reset(panel) != ESP_OK ||
        esp_lcd_panel_init(panel) != ESP_OK)
        return false;
    if (r.displayEnable >= 0 && esp_lcd_panel_disp_on_off(panel, true) != ESP_OK)
        return false;
    void *buffers[2]{};
    if (esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &buffers[0], &buffers[1]) != ESP_OK)
        return false;
    front = static_cast<uint16_t *>(buffers[0]);
    back = static_cast<uint16_t *>(buffers[1]);
    std::memset(front, 0, size_t(p.width) * p.height * 2);
    std::memset(back, 0, size_t(p.width) * p.height * 2);
    esp_lcd_rgb_panel_event_callbacks_t callbacks{};
    callbacks.on_vsync = complete;
    if (esp_lcd_rgb_panel_register_event_callbacks(panel, &callbacks, nullptr) != ESP_OK)
        return false;
    first = p.height;
    return XTouchPort::prepareBacklight();
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    if (asleep)
        return;
    const auto &p = XTouchPort::profile();
    if (waiting) {
        redraw = true;
        return;
    }
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > p.width || y + h > p.height)
        return;
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            back[(y + row) * p.width + x + col] = pixels[row * w + col] ^ (inverted ? 0xffff : 0);
    first = std::min(first, y);
    end = std::max(end, y + h);
}
void board_flush() {
    const auto &p = XTouchPort::profile();
    if (end <= first)
        return;
    if (!waiting) {
        esp_cache_msync(back + size_t(first) * p.width, size_t(end - first) * p.width * 2,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
        submitted = frames;
        if (esp_lcd_panel_draw_bitmap(panel, 0, 0, p.width, p.height, back) != ESP_OK)
            return;
        waiting = true;
    }
    const uint32_t start = millis();
    while (uint32_t(frames - submitted) < 2 && uint32_t(millis() - start) < 150)
        vTaskDelay(1);
    if (uint32_t(frames - submitted) < 2) {
        ESP_LOGW("display", "RGB vsync timeout");
        return;
    }
    waiting = false;
    std::swap(front, back);
    const size_t offset = size_t(first) * p.width, bytes = size_t(end - first) * p.width * 2;
    std::memcpy(back + offset, front + offset, bytes);
    esp_cache_msync(back + offset, bytes,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    first = p.height;
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
    if (waiting)
        return false;
    const auto &p = XTouchPort::profile();
    if (XTouchPort::rgbProfile().displayEnable >= 0) {
        if (esp_lcd_panel_disp_on_off(panel, !sleeping) != ESP_OK)
            return false;
        asleep = sleeping;
        if (!sleeping)
            lv_obj_invalidate(lv_scr_act());
        return true;
    }
    if (sleeping && p.backlight < 0 && p.power == XTouchPort::Power::Gpio) {
        std::memset(back, 0, size_t(p.width) * p.height * 2);
        first = 0;
        end = p.height;
        board_flush();
        if (waiting)
            return false;
    }
    asleep = sleeping;
    if (!sleeping)
        lv_obj_invalidate(lv_scr_act());
    return true;
}
