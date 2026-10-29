#include "board.h"
#include "display_budget.h"
#include "platform/idf/json.h"
#include "xtouch/storage.h"
#include "xtouch/paths.h"
#include <driver/spi_master.h>
#include <driver/ledc.h>
#include "features/diagnostics/display_stats.h"
#include <algorithm>
#include <cmath>

namespace {
spi_device_handle_t lcd;
using XTouchCydDisplay::DrawBytes;
using XTouchCydDisplay::DrawPixels;
portMUX_TYPE displayGuard = portMUX_INITIALIZER_UNLOCKED;
XTouchDisplayStats displayStats;
uint32_t frameStarted = 0;
bool renderStartTiming = false;
struct PendingFlush {
    spi_transaction_t transaction;
    lv_display_t *display;
    bool last;
} pendingFlush;
bool dmaPending = false;
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
uint32_t tileQueuedAt, frameDmaUs, frameSetupUs, framePixels;
uint32_t completedTotalUs, completedDmaUs, completedSetupUs, completedPixels;
#endif
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
bool forcePolling = false;
uint16_t *diagnosticDrawBuffer = nullptr;
#endif
void drain() {
    if (!dmaPending)
        return;
    spi_transaction_t *done = nullptr;
    ESP_ERROR_CHECK(spi_device_get_trans_result(lcd, &done, portMAX_DELAY));
    dmaPending = false;
}
void complete(bool last, bool inISR) {
    if (!last)
        return;
    uint32_t now = (uint32_t)esp_timer_get_time();
    if (inISR)
        portENTER_CRITICAL_ISR(&displayGuard);
    else
        portENTER_CRITICAL(&displayGuard);
    if (frameStarted) {
        displayStats.includesFirstTile = renderStartTiming;
        displayStats.lastFrameUs = now - frameStarted;
        displayStats.maxFrameUs = std::max(displayStats.maxFrameUs, displayStats.lastFrameUs);
        ++displayStats.frames;
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
        completedTotalUs = displayStats.lastFrameUs;
        completedDmaUs = frameDmaUs;
        completedSetupUs = frameSetupUs;
        completedPixels = framePixels;
#endif
        frameStarted = 0;
    }
    if (inISR)
        portEXIT_CRITICAL_ISR(&displayGuard);
    else
        portEXIT_CRITICAL(&displayGuard);
}
void renderBegin(lv_event_t *) {
    drain();
    portENTER_CRITICAL(&displayGuard);
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    uint32_t total = completedTotalUs, dma = completedDmaUs, setup = completedSetupUs,
             pixels = completedPixels;
    completedPixels = 0;
    frameDmaUs = frameSetupUs = framePixels = 0;
#endif
    frameStarted = uint32_t(esp_timer_get_time());
    renderStartTiming = true;
    portEXIT_CRITICAL(&displayGuard);
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    if (pixels >= 320 * 240) {
        Serial.printf("[CYD_FRAME] total=%u dma=%u setup=%u pixels=%u rows=16\n", unsigned(total),
                      unsigned(dma), unsigned(setup), unsigned(pixels));
        portENTER_CRITICAL(&displayGuard);
        frameStarted = uint32_t(esp_timer_get_time());
        portEXIT_CRITICAL(&displayGuard);
    }
#endif
}
void flushDone(spi_transaction_t *transaction) {
    if (transaction->user != &pendingFlush)
        return;
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    portENTER_CRITICAL_ISR(&displayGuard);
    frameDmaUs += uint32_t(esp_timer_get_time()) - tileQueuedAt;
    portEXIT_CRITICAL_ISR(&displayGuard);
#endif
    complete(pendingFlush.last, true);
    lv_display_flush_ready(pendingFlush.display);
}
float xm = 0, xc = 0, ym = 0, yc = 0;
bool calibrated = false;
uint8_t backlightLevel = 255;
void transfer(spi_device_handle_t device, const void *data, size_t bytes, void *receive = nullptr) {
    spi_transaction_t t{};
    t.length = bytes * 8;
    t.tx_buffer = data;
    t.rx_buffer = receive;
    ESP_ERROR_CHECK(spi_device_polling_transmit(device, &t));
    portENTER_CRITICAL(&displayGuard);
    ++displayStats.transactions;
    displayStats.wireBytes += bytes;
    portEXIT_CRITICAL(&displayGuard);
}
void command(uint8_t value, const uint8_t *data = nullptr, size_t bytes = 0) {
    drain();
    gpio_set_level(GPIO_NUM_2, 0);
    transfer(lcd, &value, 1);
    if (bytes) {
        gpio_set_level(GPIO_NUM_2, 1);
        transfer(lcd, data, bytes);
    }
}
void reg(uint8_t value, std::initializer_list<uint8_t> data) {
    command(value, data.begin(), data.size());
}
int adc(uint8_t command) {
    uint32_t result = 0;
    digitalWrite(33, LOW);
    // The touch controller has separate pins. GPIO SPI leaves SPI3 for the SD card.
    for (int bit = 23; bit >= 0; --bit) {
        digitalWrite(32, bit >= 16 ? ((command >> (bit - 16)) & 1) : 0);
        digitalWrite(25, HIGH);
        delayMicroseconds(1);
        result = (result << 1) | digitalRead(39);
        digitalWrite(25, LOW);
        delayMicroseconds(1);
    }
    digitalWrite(33, HIGH);
    return (result >> 3) & 4095;
}
bool raw(int &x, int &y) {
    if (gpio_get_level(GPIO_NUM_36))
        return false;
    const int pressure = adc(0xb1) + 4095 - adc(0xc1);
    if (pressure < 400) {
        adc(0xd0);
        return false;
    }
    // Same rotation-1 coordinate convention as the existing CYD calibration.
    int ys[3], xs[3];
    for (int n = 0; n < 3; ++n) {
        xs[n] = adc(0x91);
        ys[n] = adc(0xd1);
    }
    adc(0xd0);
    std::sort(xs, xs + 3);
    std::sort(ys, ys + 3);
    x = xs[1];
    y = ys[1];
    return true;
}
void marker(int x, int y) {
    uint16_t line[21];
    std::fill(line, line + 21, uint16_t(0xffff));
    board_pixels(x - 10, y, 21, 1, line);
    board_pixels(x, y - 10, 1, 21, line);
}
} // namespace
bool board_display_begin() {
    pinMode(2, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(36, INPUT);
    for (int pin : {4, 16, 17}) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
    spi_bus_config_t bus{};
    bus.mosi_io_num = 13;
    bus.miso_io_num = -1;
    bus.sclk_io_num = 14;
    bus.quadwp_io_num = bus.quadhd_io_num = -1;
    bus.max_transfer_sz = DrawBytes;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    spi_device_interface_config_t device{};
    device.clock_speed_hz = 40000000;
    device.spics_io_num = 15;
    device.queue_size = 1;
    device.post_cb = flushDone;
    if (spi_bus_add_device(SPI2_HOST, &device, &lcd) != ESP_OK)
        return false;
    pinMode(32, OUTPUT);
    pinMode(25, OUTPUT);
    pinMode(33, OUTPUT);
    pinMode(39, INPUT);
    digitalWrite(33, HIGH);
    digitalWrite(25, LOW);
    digitalWrite(12, LOW);
    delay(20);
    digitalWrite(12, HIGH);
    delay(150);
    command(0x01);
    delay(150);
    reg(0xcf, {0, 0xc1, 0x30});
    reg(0xed, {0x64, 3, 0x12, 0x81});
    reg(0xe8, {0x85, 0, 0x78});
    reg(0xcb, {0x39, 0x2c, 0, 0x34, 2});
    reg(0xf7, {0x20});
    reg(0xea, {0, 0});
    // Preserve the board's previously selected ILI9341_2 analog profile.
    // Panel-specific analog settings are applied through the native SPI driver.
    reg(0xc0, {0x10});
    reg(0xc1, {0x00});
    reg(0xc5, {0x30, 0x30});
    reg(0xc7, {0xb7});
    reg(0x36, {0x28});
    reg(0x3a, {0x55});
    reg(0xb1, {0, 0x1a});
    reg(0xb6, {8, 0x82, 0x27});
    reg(0xf2, {0});
    reg(0x26, {1});
    reg(0xe0, {0x0f, 0x2a, 0x28, 8, 0x0e, 8, 0x54, 0xa9, 0x43, 0x0a, 0x0f, 0, 0, 0, 0});
    reg(0xe1, {0, 0x15, 0x17, 7, 0x11, 6, 0x2b, 0x56, 0x3c, 5, 0x10, 0x0f, 0x3f, 0x3f, 0x0f});
    command(0x11);
    delay(120);
    command(0x29);
    // A dedicated reference-clock timer keeps backlight PWM independent of APB/DFS.
    ledc_timer_config_t timer{};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.timer_num = LEDC_TIMER_1;
    timer.duty_resolution = LEDC_TIMER_8_BIT;
    timer.freq_hz = 1000;
    timer.clk_cfg = LEDC_USE_REF_TICK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    ledc_channel_config_t backlight{};
    backlight.gpio_num = 21;
    backlight.speed_mode = LEDC_LOW_SPEED_MODE;
    backlight.channel = LEDC_CHANNEL_0;
    backlight.timer_sel = LEDC_TIMER_1;
    ESP_ERROR_CHECK(ledc_channel_config(&backlight));
    Serial.println("[PANEL] native ILI9341_2 power/VCOM/gamma profile");
    Serial.println("[PANEL] LCD SPI 40 MHz");
    return true;
}
void board_display_ready(lv_display_t *display) {
    lv_display_add_event_cb(display, renderBegin, LV_EVENT_RENDER_START, nullptr);
}
bool board_draw_buffers(uint16_t **first, uint16_t **second, uint32_t *count) {
    // A measured partial-buffer budget; keep the DMA-owned region immutable until ready.
    if (!first || !second || !count)
        return false;
    *count = 0;
    *second = nullptr;
    *first =
        static_cast<uint16_t *>(heap_caps_malloc(DrawBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (!*first)
        return false;
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    diagnosticDrawBuffer = *first;
#endif
    *count = DrawPixels;
    return true;
}
XTouchDisplayStats xtouch_board_display_stats() {
    portENTER_CRITICAL(&displayGuard);
    auto snapshot = displayStats;
    portEXIT_CRITICAL(&displayGuard);
    snapshot.available = true;
    snapshot.dma = true;
    snapshot.bufferBytes = DrawBytes;
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    snapshot.dma = !forcePolling;
#endif
    return snapshot;
}
bool board_pixels_async(lv_display_t *display, int x, int y, int w, int h, const uint16_t *pixels) {
    if (!XTouchCydDisplay::fitsDma(x, y, w, h))
        return false;
    drain();
    const bool last = lv_display_flush_is_last(display);
    uint32_t now = (uint32_t)esp_timer_get_time();
    portENTER_CRITICAL(&displayGuard);
    if (!frameStarted)
        frameStarted = now;
    ++displayStats.flushes;
    portEXIT_CRITICAL(&displayGuard);
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    if (forcePolling) {
        board_pixels(x, y, w, h, pixels);
        complete(last, false);
        lv_display_flush_ready(display);
        return true;
    }
#endif
    reg(0x2a, {uint8_t(x >> 8), uint8_t(x), uint8_t((x + w - 1) >> 8), uint8_t(x + w - 1)});
    reg(0x2b, {uint8_t(y >> 8), uint8_t(y), uint8_t((y + h - 1) >> 8), uint8_t(y + h - 1)});
    command(0x2c);
    gpio_set_level(GPIO_NUM_2, 1);
    // LVGL lends its writable, DMA-capable partial buffer until flush-ready.
    auto *wire = const_cast<uint16_t *>(pixels);
    for (int i = 0; i < w * h; ++i)
        wire[i] = __builtin_bswap16(wire[i]);
    pendingFlush = {};
    pendingFlush.display = display;
    pendingFlush.last = last;
    auto &transaction = pendingFlush.transaction;
    transaction.length = w * h * 16;
    transaction.tx_buffer = wire;
    transaction.user = &pendingFlush;
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
    tileQueuedAt = uint32_t(esp_timer_get_time());
    portENTER_CRITICAL(&displayGuard);
    frameSetupUs += tileQueuedAt - now;
    framePixels += unsigned(w * h);
    portEXIT_CRITICAL(&displayGuard);
#endif
    esp_err_t result = spi_device_queue_trans(lcd, &transaction, portMAX_DELAY);
    if (result != ESP_OK) {
        for (int i = 0; i < w * h; ++i)
            wire[i] = __builtin_bswap16(wire[i]);
        portENTER_CRITICAL(&displayGuard);
        ++displayStats.failedTransfers;
        portEXIT_CRITICAL(&displayGuard);
        board_pixels(x, y, w, h, pixels);
        complete(last, false);
        lv_display_flush_ready(display);
        return true;
    }
    dmaPending = true;
    portENTER_CRITICAL(&displayGuard);
    ++displayStats.transactions;
    displayStats.wireBytes += w * h * 2;
    portEXIT_CRITICAL(&displayGuard);
    return true;
}
void board_pixels(int x, int y, int w, int h, const uint16_t *pixels) {
    if (!XTouchCydDisplay::validRegion(x, y, w, h))
        return;
    reg(0x2a, {uint8_t(x >> 8), uint8_t(x), uint8_t((x + w - 1) >> 8), uint8_t(x + w - 1)});
    reg(0x2b, {uint8_t(y >> 8), uint8_t(y), uint8_t((y + h - 1) >> 8), uint8_t(y + h - 1)});
    command(0x2c);
    digitalWrite(2, HIGH);
    // LVGL stores native RGB565; the panel expects most-significant byte first.
    uint16_t tile[320];
    size_t left = size_t(w) * h;
    while (left) {
        unsigned n = std::min(left, size_t(320));
        for (unsigned i = 0; i < n; ++i)
            tile[i] = __builtin_bswap16(*pixels++);
        transfer(lcd, tile, n * 2);
        left -= n;
    }
}
void board_flush() {
    drain();
}
void board_brightness(uint8_t value) {
    backlightLevel = value;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, value));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
void board_display_test_buffers(bool doubled) {
    drain();
    if (!diagnosticDrawBuffer)
        return;
    auto *display = lv_display_get_default();
    lv_display_set_buffers(display, diagnosticDrawBuffer,
                           doubled ? diagnosticDrawBuffer + DrawPixels / 2 : nullptr,
                           doubled ? DrawBytes / 2 : DrawBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_obj_invalidate(lv_screen_active());
}
void board_display_test_polling(bool enabled) {
    drain();
    forcePolling = enabled;
}
void board_display_test(bool frozen, bool directLight) {
    auto *refresh = lv_display_get_refr_timer(lv_display_get_default());
    if (frozen)
        lv_timer_pause(refresh);
    else {
        lv_timer_resume(refresh);
        lv_obj_invalidate(lv_screen_active());
    }
    if (directLight) {
        ESP_ERROR_CHECK(gpio_reset_pin(GPIO_NUM_21));
        ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_21, 1));
    } else {
        ESP_ERROR_CHECK(ledc_set_pin(21, LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        board_brightness(backlightLevel);
    }
    Serial.printf("[DISPLAY TEST] frozen=%u directLight=%u duty=%lu frequency=%lu\n", frozen,
                  directLight, (unsigned long)ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0),
                  (unsigned long)ledc_get_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1));
}
#endif
bool board_display_sleep(bool sleeping) {
    command(sleeping ? 0x28 : 0x11);
    delay(120);
    command(sleeping ? 0x10 : 0x29);
    return true;
}
void board_invert(bool invert) {
    command(invert ? 0x21 : 0x20);
}
bool board_touch(int &x, int &y) {
    int rx, ry;
    if (!calibrated || !raw(rx, ry))
        return false;
    x = std::clamp(int(std::lround(rx * xm + xc)), 0, 319);
    y = std::clamp(int(std::lround(ry * ym + yc)), 0, 239);
    return true;
}
void board_touch_setup() {
    auto doc = xtouch_storage_read(xtouch_paths_touch, 512);
    xm = doc["xCalM"] | 0.f;
    xc = doc["xCalC"] | 0.f;
    ym = doc["yCalM"] | 0.f;
    yc = doc["yCalC"] | 0.f;
    calibrated = std::isfinite(xm) && std::isfinite(ym) && std::isfinite(xc) && std::isfinite(yc) &&
                 std::abs(xm) > 0.001f && std::abs(ym) > 0.001f;
    if (calibrated)
        return;
    // First boot only: collect two targets, yielding while waiting for the user.
    int x1, y1, x2, y2;
    auto point = [](int x, int y, int &rx, int &ry) {
        marker(x, y);
        while (raw(rx, ry))
            delay(10);
        while (!raw(rx, ry))
            delay(10);
        delay(50);
        raw(rx, ry);
        while (gpio_get_level(GPIO_NUM_36) == 0)
            delay(10);
    };
    point(20, 20, x1, y1);
    point(300, 220, x2, y2);
    if (std::abs(x2 - x1) < 500 || std::abs(y2 - y1) < 500)
        return;
    xm = 280.f / (x2 - x1);
    xc = 20 - x1 * xm;
    ym = 200.f / (y2 - y1);
    yc = 20 - y1 * ym;
    StaticJsonDocument<256> saved;
    saved["xCalM"] = xm;
    saved["xCalC"] = xc;
    saved["yCalM"] = ym;
    saved["yCalC"] = yc;
    xtouch_storage_write(xtouch_paths_touch, saved);
    calibrated = true;
}
