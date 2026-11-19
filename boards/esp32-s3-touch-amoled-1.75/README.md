# Waveshare ESP32-S3-Touch-AMOLED-1.75

## 硬件 / Hardware

| 项目 / Item | 配置 / Configuration |
|---|---|
| 主控 / MCU | ESP32-S3R8 |
| Flash / PSRAM | 16 MB / 8 MB Octal |
| 屏幕 / Display | 1.75-inch round AMOLED, 466 × 466, CO5300 QSPI |
| 触摸 / Touch | CST9217 capacitive, I²C |
| 声音 / Audio | ES8311 DAC, NS4150 amplifier |
| 存储 / Storage | microSD, 1-bit SDMMC |
| 亮度 / Brightness | AMOLED DCS brightness; no PWM backlight |
| 状态灯 / Status LED | No application-controlled RGB indicator |

## 构建 / Build

在 ESP-IDF 6.1 环境中，从仓库根目录执行：

From the repository root in an ESP-IDF 6.1 environment:

```sh
python boards/build.py amoled175 --layout round --assets large
```

该目标默认使用圆屏布局、大屏资源，以及 S3 的共用可选功能。蓝牙配网默认关闭。屏幕布局与资源大小独立，不能用其他主板的固件替代。构建后使用 `artifacts/` 中匹配本板名称的文件；应用 OTA 保留双槽，各 6 MiB。

This target defaults to round composition, large assets and the shared S3 feature selection. BLE provisioning is off by default. Layout and asset size are independent; firmware for another board is incompatible. Use the matching board files in the build's `artifacts/` directory. Application OTA uses two 6 MiB slots.

## 引脚 / Pins

| 接口 / Interface | GPIO |
|---|---|
| I²C SDA / SCL | 15 / 14 |
| LCD CS / SCLK / D0–D3 / RESET | 12 / 38 / 4, 5, 6, 7 / 39 |
| Touch RESET / IRQ | 40 / 11 |
| Audio BCLK / WS / DOUT / MCLK / AMP | 9 / 45 / 8 / 42 / 46 |
| SD CMD / CLK / D0 | 1 / 2 / 3 |
| BOOT | 0, active low |

BOOT 短按和普通熄屏沿用共用待机逻辑；深睡使用 BOOT 唤醒，不声明触摸深睡唤醒。传感器、RTC 和麦克风未作为新增功能启用。电源键和电池管理保持板卡默认行为。

BOOT short press and display standby use the shared power logic. BOOT wakes deep sleep; touch wake from deep sleep is not advertised. Onboard motion sensing, RTC and microphone are not enabled as additional features. The power key and battery management retain the board's default behavior.

## 硬件资料 / Hardware documentation

[Waveshare 官方文档 / Official documentation](https://docs.waveshare.net/ESP32-S3-Touch-AMOLED-1.75/)

Panel initialization provenance and component licenses are recorded in [NOTICE.md](NOTICE.md).
