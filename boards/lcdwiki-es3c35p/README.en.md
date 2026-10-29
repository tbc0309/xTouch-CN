# LCDWIKI ES3C35P · ESP32-S3 3.5 inch

[简体中文](README.md) | English

Native ESP-IDF 6.1 / LVGL 9.6.0 support with the shared S3 feature modules and UI, using large MDI resources by default. Supports capacitive touch, speaker, RGB LED, touch wake from connected standby and configuration-preserving OTA.

## Hardware

| Function | Pins and configuration |
|---|---|
| Memory | ESP32-S3, 16 MB Flash, 8 MB Octal PSRAM |
| Display | ST77922, native 320×480, software landscape 480×320, 40 MHz QSPI |
| QSPI | CS10, CLK12, D0=11, D1=13, D2=14, D3=9 |
| Backlight | GPIO41, 20 kHz PWM |
| Touch | I²C address0x55; SDA38/SCL39, RST48/INT47; complete point packet read |
| Speaker | ES8311 address0x18; MCLK17, BCLK18, WS21, DOUT15; active-low PA GPIO1 |
| Status LED | One WS2812, GPIO40, RMT |
| SD | 4-bit SDMMC: CLK5, CMD4, D0=6, D1=7, D2=2, D3=3 |
| Controls | BOOT GPIO0, RESET; USB Serial/JTAG console |

Touch wakes connected standby. INT47 is not an RTC pin: optional deep sleep uses BOOT to wake. Microphone and battery measurement are outside this port's scope.

## Build and identity

```sh
python boards/build.py es3c35p
```

`esp3c35p` is also accepted. Artifacts are `lcdwiki-es3c35p.bin` and `lcdwiki-es3c35p-full.bin` under `boards/build-es3c35p/artifacts/`. The 16 MB layout has two 6 MiB OTA slots. Use the application image for OTA and the full image for USB installation.

This board cannot use Waveshare S3 firmware despite sharing the same MCU. Packaging and application updates check the chip and project identity `xtouch_es3c35p`.

## Display and transport

[Vendor information](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display) · [Hardware initialization license](LICENSE.hardware-init)

Rotation and RGB565 byte swapping happen during PSRAM composition because this glass does not reliably support MADCTL axis swapping. Queued DMA data has a separate buffer. Touch coordinates follow the display orientation; sounds and the status LED have independent controls.

QSPI does not DMA directly from PSRAM: IDF stages transfers through at most two internal DMA chunks (a requested 5 KiB limit rounds up to at most 8 KiB at the IDF descriptor boundary) to avoid TX underflow from external-memory bandwidth contention. PSRAM composition remains separate from in-flight data.
