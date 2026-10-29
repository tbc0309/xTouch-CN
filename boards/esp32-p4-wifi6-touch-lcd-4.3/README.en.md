# ESP32-P4-WIFI6-Touch-LCD-4.3

[简体中文](README.md) | English

This target requires P4 v3.x, minimum revision v3.0. It uses an 800×480 display and large MDI resources by default, with a Hosted 3 host for the board's wireless communication.

| Peripheral | Wiring / configuration |
|---|---|
| LCD | ST7701 MIPI-DSI, two lanes at 500 Mbps; native 480×800; 30 MHz pixel clock |
| LCD reset / backlight | GPIO27 / GPIO26; 5 kHz, 10-bit backlight PWM |
| Touch | GT911; I²C SDA 7, SCL 8; polled at 0x5D / 0x14, no separate INT/RST |
| Wi-Fi | C6 / Hosted, SDIO slot 1; CLK18, CMD19, D0..3=14..17, reset54 |
| SDMMC | Slot 0, four-bit; CLK43, CMD44, D0..3=39..42; separate from wireless SDIO |
| SD power | GPIO45 active-low switch and LDO4 |
| Storage | 32 MB Flash; two 12 MiB OTA slots and 7.875 MiB SPIFFS; 32 MB PSRAM at 200 MHz |
| Buttons | BOOT GPIO35 toggles connected standby; POWER switches hardware power; RESET restarts hardware |
| Audio | ES8311; MCLK13, BCLK12, LRCK10, DOUT9, amplifier enable53; I²C shared with touch |

## Build and operation

Activate ESP-IDF 6.1 and run `python boards/build.py p4` from the repository root. The target uses native ESP-IDF and LVGL 9.6.0 without the Arduino framework. See [build options](../../docs/DEVELOPMENT.en.md#options) for module and resource selection.

Display, wireless SDIO, SD storage and low-power touch have separate lifecycles. Capacitive touch does not use CYD calibration. Connected standby retains networking. Automatic deep sleep and touch/warning sounds are off by default; deep sleep is a separate optional module. Hardware power/charging indicators are not controllable RGB status LEDs.

Use this board's application image for browser/OTA updates and its full image for USB installation. Different board images are not interchangeable.

[Installation and images](../../docs/INSTALLATION.en.md#install) · [Camera support](../CAMERA.md#en) · [Hardware](../../docs/HARDWARE.en.md)
