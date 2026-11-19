#include "board.h"
#include "platform/idf/i2c.h"
#include <driver/spi_master.h>
#include <esp_lcd_co5300.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_touch_cst9217.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/semphr.h>

namespace {
constexpr int W = 466, H = 466, Rows = 24;
esp_lcd_panel_handle_t panel;
esp_lcd_panel_io_handle_t io;
esp_lcd_touch_handle_t touch;
SemaphoreHandle_t done;
uint16_t *draw;
bool pending;
void wait() {
    if (pending) {
        xSemaphoreTake(done, portMAX_DELAY);
        pending = false;
    }
}
bool complete(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(done, &wake);
    return wake == pdTRUE;
}
// Panel glass offsets and electrical initialization from the board vendor.
const uint8_t data[][4] = {{0x20}, {0x10}, {0xa0}, {0},    {0x80},          {0x55},
                           {0},    {0x20}, {0xff}, {0xff}, {0, 6, 1, 0xd7}, {0, 0, 1, 0xd1}};
const co5300_lcd_init_cmd_t init[] = {
    {0xfe, data[0], 1, 0},   {0x19, data[1], 1, 0},  {0x1c, data[2], 1, 0},
    {0xfe, data[3], 1, 0},   {0xc4, data[4], 1, 0},  {0x3a, data[5], 1, 0},
    {0x35, data[6], 1, 0},   {0x53, data[7], 1, 0},  {0x51, data[8], 1, 0},
    {0x63, data[9], 1, 0},   {0x2a, data[10], 4, 0}, {0x2b, data[11], 4, 600},
    {0x11, nullptr, 0, 600}, {0x29, nullptr, 0, 0}};
void round_area(lv_event_t *event) {
    auto *a = static_cast<lv_area_t *>(lv_event_get_param(event));
    a->x1 &= ~1;
    a->y1 &= ~1;
    a->x2 |= 1;
    a->y2 |= 1;
    if (a->x2 >= W)
        a->x2 = W - 1;
    if (a->y2 >= H)
        a->y2 = H - 1;
}
} // namespace
bool board_display_begin() {
    if (!xtouch_i2c.begin(15, 14, 400000))
        return false;
    done = xSemaphoreCreateBinary();
    draw = static_cast<uint16_t *>(
        heap_caps_malloc(W * Rows * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (!done || !draw)
        return false;
    spi_bus_config_t bus{};
    bus.sclk_io_num = 38;
    bus.data0_io_num = 4;
    bus.data1_io_num = 5;
    bus.data2_io_num = 6;
    bus.data3_io_num = 7;
    bus.data4_io_num = bus.data5_io_num = bus.data6_io_num = bus.data7_io_num = -1;
    bus.max_transfer_sz = W * Rows * 2;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    esp_lcd_panel_io_spi_config_t cfg{};
    cfg.cs_gpio_num = GPIO_NUM_12;
    cfg.dc_gpio_num = GPIO_NUM_NC;
    cfg.on_color_trans_done = complete;
    cfg.trans_queue_depth = 1;
    cfg.pclk_hz = 40000000;
    cfg.lcd_cmd_bits = 32;
    cfg.lcd_param_bits = 8;
    cfg.flags.quad_mode = true;
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &cfg, &io) != ESP_OK)
        return false;
    co5300_vendor_config_t vendor{};
    vendor.init_cmds = init;
    vendor.init_cmds_size = sizeof(init) / sizeof(init[0]);
    vendor.flags.use_qspi_interface = 1;
    esp_lcd_panel_dev_config_t dev{};
    dev.reset_gpio_num = GPIO_NUM_39;
    dev.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    dev.bits_per_pixel = 16;
    dev.vendor_config = &vendor;
    if (esp_lcd_new_panel_co5300(io, &dev, &panel) != ESP_OK)
        return false;
    if (esp_lcd_panel_set_gap(panel, 6, 0) != ESP_OK || esp_lcd_panel_reset(panel) != ESP_OK ||
        esp_lcd_panel_init(panel) != ESP_OK)
        return false;
    if (esp_lcd_panel_disp_on_off(panel, true) != ESP_OK)
        return false;
    esp_lcd_panel_io_handle_t tio = nullptr;
    esp_lcd_panel_io_i2c_config_t tc{};
    tc.dev_addr = ESP_LCD_TOUCH_IO_I2C_CST9217_ADDRESS;
    tc.control_phase_bytes = 1;
    tc.lcd_cmd_bits = 8;
    tc.lcd_param_bits = 8;
    tc.scl_speed_hz = 400000;
    tc.flags.disable_control_phase = 1;
    if (esp_lcd_new_panel_io_i2c(xtouch_i2c.native_handle(), &tc, &tio) != ESP_OK)
        return false;
    esp_lcd_touch_config_t t{};
    t.x_max = W;
    t.y_max = H;
    t.rst_gpio_num = GPIO_NUM_40;
    t.int_gpio_num = GPIO_NUM_11;
    t.levels.reset = 0;
    t.levels.interrupt = 0;
    t.flags.mirror_x = 1;
    t.flags.mirror_y = 1;
    if (esp_lcd_touch_new_i2c_cst9217(tio, &t, &touch) != ESP_OK)
        return false;
    ESP_LOGI("amoled175", "CO5300 466x466 QSPI 40MHz; CST9217 ready");
    return true;
}
bool board_draw_buffers(uint16_t **first, uint16_t **second, uint32_t *pixels) {
    *first = draw;
    *second = nullptr;
    *pixels = W * Rows;
    return draw != nullptr;
}
void board_display_ready(lv_display_t *display) {
    lv_display_add_event_cb(display, round_area, LV_EVENT_INVALIDATE_AREA, nullptr);
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    wait();
    auto *p = const_cast<uint16_t *>(pixels);
    for (int i = 0; i < w * h; ++i)
        p[i] = uint16_t((p[i] << 8) | (p[i] >> 8));
    pending = true;
    if (esp_lcd_panel_draw_bitmap(panel, x, y, x + w, y + h, p) != ESP_OK)
        pending = false;
    wait();
}
void board_flush() {
    wait();
}
bool board_touch(int &x, int &y) {
    uint16_t tx, ty;
    uint8_t count = 0;
    if (!touch || esp_lcd_touch_read_data(touch) != ESP_OK)
        return false;
    if (!esp_lcd_touch_get_coordinates(touch, &tx, &ty, nullptr, &count, 1) || !count || tx >= W ||
        ty >= H)
        return false;
    x = tx;
    y = ty;
    return true;
}
void board_brightness(uint8_t value) {
    wait();
    if (io)
        esp_lcd_panel_io_tx_param(io, 0x02005100, &value, 1);
}
bool board_display_sleep(bool sleeping) {
    wait();
    return panel && esp_lcd_panel_disp_on_off(panel, !sleeping) == ESP_OK;
}
void board_invert(bool invert) {
    wait();
    if (panel)
        esp_lcd_panel_invert_color(panel, invert);
}
