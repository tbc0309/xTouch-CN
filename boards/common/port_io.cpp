#include "board.h"
#include "port_profile.h"
#include "port_geometry.h"
#include "port_touch_packet.h"
#include "touch_contact.h"
#include "platform/idf/i2c.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <algorithm>
namespace {
TouchContact contact;
bool backlightReady;
XTouchPort::Touch touchKind;
uint8_t touchAddress;
bool writeRegister(uint8_t address, uint8_t index, uint8_t value) {
    const uint8_t bytes[] = {index, value};
    return xtouch_i2c.transfer(address, bytes, sizeof(bytes), nullptr, 0) == ESP_OK;
}
bool writeExpander(uint8_t index, uint16_t value) {
    const auto &p = XTouchPort::profile();
    const uint8_t bytes[] = {index, uint8_t(value), uint8_t(value >> 8)};
    return xtouch_i2c.transfer(0x24, bytes, p.expanderWidth == 16 ? 3 : 2, nullptr, 0) == ESP_OK;
}
bool reg(uint8_t address, uint16_t index, unsigned indexBytes, uint8_t *out, size_t n) {
    uint8_t bytes[] = {uint8_t(index >> 8), uint8_t(index)};
    return xtouch_i2c.transfer(address, bytes + 2 - indexBytes, indexBytes, out, n) == ESP_OK;
}
} // namespace
namespace XTouchPort {
bool preparePeripherals() {
    const auto &p = profile();
    if (!xtouch_i2c.begin(p.sda, p.scl, 400000))
        return false;
    if (p.power == Power::Ch32) {
        if (!writeExpander(2, uint16_t(p.expanderDirection)) ||
            !writeExpander(3, uint16_t(p.expanderOutput & ~2U)))
            return false;
        delay(200);
        gpio_set_direction(gpio_num_t(p.touchInterrupt), GPIO_MODE_OUTPUT);
        gpio_set_level(gpio_num_t(p.touchInterrupt), 0);
        delay(200);
        if (!writeExpander(3, uint16_t(p.expanderOutput)))
            return false;
        delay(200);
        gpio_set_direction(gpio_num_t(p.touchInterrupt), GPIO_MODE_INPUT);
    }
    if (p.power == Power::Tca9554) {
        if (!writeRegister(0x20, 3, 0x7c) || !writeRegister(0x20, 1, 0))
            return false;
        delay(100);
        if (!writeRegister(0x20, 1, 0x82))
            return false;
        const uint8_t pmic[][2] = {{0x22, 6},  {0x27, 0x10}, {0x80, 1},  {0x90, 0},  {0x91, 0},
                                   {0x82, 18}, {0x92, 28},   {0x96, 10}, {0x97, 23}, {0x90, 0x31}};
        for (const auto &item : pmic)
            if (!writeRegister(0x34, item[0], item[1]))
                return false;
        delay(100);
    }
    if (p.power == Power::CoreS3) {
        const uint8_t reg90 = 0x90;
        uint8_t current;
        if (xtouch_i2c.transfer(0x34, &reg90, 1, &current, 1) != ESP_OK ||
            !writeRegister(0x34, 0x90, current | 0xb4))
            return false;
        const uint8_t pmic[][2] = {{0x99, 25},   {0x97, 28}, {0x69, 0x35}, {0x30, 0x3f},
                                   {0x90, 0xbf}, {0x94, 28}, {0x95, 28}};
        for (const auto &item : pmic)
            if (!writeRegister(0x34, item[0], item[1]))
                return false;
        const uint8_t expander[][2] = {{2, 7},       {3, 0x8f},    {4, 0x18},   {5, 0x0c},
                                       {0x11, 0x10}, {0x12, 0xff}, {0x13, 0xff}};
        for (const auto &item : expander)
            if (!writeRegister(0x58, item[0], item[1]))
                return false;
        if (!writeRegister(0x58, 2, 3))
            return false;
        delay(10);
        if (!writeRegister(0x58, 2, 7) || !writeRegister(0x58, 3, 0x81))
            return false;
        delay(20);
        if (!writeRegister(0x58, 3, 0x83))
            return false;
        delay(50);
    }
    if (p.expander >= 0) {
        uint8_t bytes[] = {1, uint8_t(p.expanderOutput)};
        if (xtouch_i2c.transfer(p.expander, bytes, 2, nullptr, 0) != ESP_OK)
            return false;
        bytes[0] = 3;
        bytes[1] = uint8_t(p.expanderDirection);
        if (xtouch_i2c.transfer(p.expander, bytes, 2, nullptr, 0) != ESP_OK)
            return false;
    }
    if (p.touchReset >= 0) {
        gpio_set_direction(gpio_num_t(p.touchReset), GPIO_MODE_OUTPUT);
        gpio_set_level(gpio_num_t(p.touchReset), 0);
        delay(20);
        gpio_set_level(gpio_num_t(p.touchReset), 1);
        delay(100);
    }
    touchKind = p.touch;
    touchAddress = p.touchAddress;
    uint8_t probe[4];
    if (touchKind == Touch::FtOrGt) {
        if (reg(0x38, 0xa3, 1, probe, 1)) {
            touchKind = Touch::Ft6x36;
            touchAddress = 0x38;
        } else {
            touchKind = Touch::Gt911;
            touchAddress = 0x5d;
        }
    }
    if ((touchKind == Touch::Gt911 || touchKind == Touch::Gt1151) &&
        !reg(touchAddress, 0x8140, 2, probe, 4)) {
        touchAddress = touchAddress == 0x5d ? 0x14 : 0x5d;
        if (!reg(touchAddress, 0x8140, 2, probe, 4))
            return false;
    }
    return true;
}
} // namespace XTouchPort
namespace XTouchPort {
bool prepareBacklight() {
    const auto &p = profile();
    if (p.backlight >= 0) {
        ledc_timer_config_t timer{};
        timer.speed_mode = LEDC_LOW_SPEED_MODE;
        timer.duty_resolution = LEDC_TIMER_10_BIT;
        timer.timer_num = LEDC_TIMER_1;
        timer.freq_hz = 20000;
        timer.clk_cfg = LEDC_AUTO_CLK;
        ledc_channel_config_t channel{};
        channel.gpio_num = p.backlight;
        channel.speed_mode = LEDC_LOW_SPEED_MODE;
        channel.channel = LEDC_CHANNEL_0;
        channel.timer_sel = LEDC_TIMER_1;
        channel.flags.output_invert = p.backlightInvert;
        backlightReady =
            ledc_timer_config(&timer) == ESP_OK && ledc_channel_config(&channel) == ESP_OK;
        if (!backlightReady)
            return false;
    }
    return true;
}
void amplifier(bool enabled) {
    const auto &p = profile();
    if (p.power == Power::Ch32 && p.expanderWidth == 16) {
        const uint8_t index = 3;
        uint8_t current[2]{};
        if (xtouch_i2c.transfer(0x24, &index, 1, current, 2) != ESP_OK)
            return;
        const uint16_t value = current[0] | uint16_t(current[1]) << 8;
        writeExpander(3, uint16_t((value & ~8U) | (enabled ? 8U : 0U)));
        return;
    }
    int address = p.power == Power::Ch32 ? 0x24 : p.power == Power::Tca9554 ? 0x20 : p.expander;
    if (address < 0)
        return;
    const uint8_t index = p.power == Power::Ch32 ? 3 : 1;
    const uint8_t bit = p.power == Power::Ch32                            ? 8
                        : p.power == Power::Tca9554                       ? 128
                        : p.power == Power::Gpio && p.panel == Panel::Rgb ? 1
                                                                          : 2;
    uint8_t value;
    if (xtouch_i2c.transfer(address, &index, 1, &value, 1) != ESP_OK)
        return;
    writeRegister(address, index, uint8_t((value & ~bit) | (enabled ? bit : 0)));
}
} // namespace XTouchPort
void board_brightness(uint8_t level) {
    if (XTouchPort::profile().power == XTouchPort::Power::CoreS3) {
        // The PMIC controls LCD voltage; use its board-specified 20..23 range.
        writeRegister(0x34, 0x99, uint8_t((uint32_t(level) * 100 / 255 + 641) >> 5));
    } else if (XTouchPort::profile().power == XTouchPort::Power::Nano) {
        writeRegister(0x45, 0x96, uint8_t(uint32_t(level) * 100 / 255));
    } else if (XTouchPort::profile().power == XTouchPort::Power::Ch32) {
        writeRegister(0x24, 5, uint8_t(255 - level));
    } else if (backlightReady) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, uint32_t(level) * 1023 / 255);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
}
bool board_touch(int &x, int &y) {
    const auto &p = XTouchPort::profile();
    uint8_t data[8]{};
    bool pressed = false;
    int nx = 0, ny = 0;
    if (touchKind == XTouchPort::Touch::Ft6x36) {
        if (!reg(touchAddress, 2, 1, data, 5))
            return contact.read(x, y);
        if ((data[0] & 15) > 2)
            return contact.read(x, y);
        pressed = (data[0] & 15) > 0 && (data[0] & 15) <= 2;
        nx = (data[1] & 15) * 256 + data[2];
        ny = (data[3] & 15) * 256 + data[4];
    } else if (touchKind == XTouchPort::Touch::Gt1151) {
        if (!reg(touchAddress, 0x814e, 2, data, 1))
            return contact.read(x, y);
        const unsigned count = data[0] & 15;
        uint8_t all[83]{};
        if (count > 0 && count <= 10 && !reg(touchAddress, 0x814e, 2, all, 3 + count * 8))
            return contact.read(x, y);
        const uint8_t ack[] = {0x81, 0x4e, 0};
        xtouch_i2c.transfer(touchAddress, ack, 3, nullptr, 0);
        const auto packet = count == 0   ? XTouchPort::TouchPacket::Released
                            : count > 10 ? XTouchPort::TouchPacket::Ignore
                                         : XTouchPort::gt1151Packet(all, 3 + count * 8, nx, ny);
        if (packet == XTouchPort::TouchPacket::Ignore)
            return contact.read(x, y);
        pressed = packet == XTouchPort::TouchPacket::Pressed;
    } else if (touchKind == XTouchPort::Touch::Axs15231b) {
        const uint8_t command[] = {0xb5, 0xab, 0xa5, 0x5a, 0, 0, 0, 8, 0, 0, 0};
        uint8_t all[8]{};
        if (xtouch_i2c.transfer(touchAddress, command, sizeof(command), nullptr, 0) != ESP_OK ||
            xtouch_i2c.transfer(touchAddress, nullptr, 0, all, sizeof(all)) != ESP_OK)
            return contact.read(x, y);
        pressed = all[1] == 1;
        nx = (all[2] & 15) * 256 + all[3];
        ny = (all[4] & 15) * 256 + all[5];
    } else if (touchKind == XTouchPort::Touch::Gt911) {
        if (!reg(touchAddress, 0x814e, 2, data, 1))
            return contact.read(x, y);
        if (!(data[0] & 0x80))
            return contact.read(x, y);
        if ((data[0] & 15) > 5) {
            const uint8_t ack[] = {0x81, 0x4e, 0};
            xtouch_i2c.transfer(touchAddress, ack, 3, nullptr, 0);
            return contact.read(x, y);
        }
        pressed = (data[0] & 15) > 0 && (data[0] & 15) <= 5;
        if (pressed) {
            if (!reg(touchAddress, 0x8150, 2, data, 4))
                return contact.read(x, y);
            nx = data[0] | data[1] << 8;
            ny = data[2] | data[3] << 8;
        }
        const uint8_t ack[] = {0x81, 0x4e, 0};
        xtouch_i2c.transfer(touchAddress, ack, 3, nullptr, 0);
    }
    if (pressed && !XTouchPort::touchPoint(p, touchKind, nx, ny))
        return contact.read(x, y);
    return contact.report(pressed, nx, ny, x, y);
}
