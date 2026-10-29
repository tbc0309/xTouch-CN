# ESP32-S3-Touch-LCD-2.8 V2

[简体中文](README.md) | English

This target supports the V2 board with CST3530 touch. It uses a 320×240 display and small original resources by default. Other board revisions and touch controllers require their own firmware.

| Peripheral | Wiring / configuration |
|---|---|
| LCD | ST7789; MOSI 45, SCLK 40, CS 42, DC 41, RST 39; 80 MHz SPI |
| Backlight | GPIO5; 20 kHz, 10-bit PWM |
| Touch | CST3530; I²C SDA 1, SCL 3, RST 2, INT 4; address 0x58 |
| SDMMC | One-bit; CLK 14, CMD 17, D0 16; D3 21 held high |
| Battery power hold | GPIO7 held high, including connected standby |
| Buttons | GPIO6 power key toggles connected standby; RESET restarts hardware |
| Storage | 16 MB Flash, 8 MB octal PSRAM; 256 KiB LVGL pool in PSRAM |
| Audio | PCM5101; BCLK48, LRCK38, DOUT47; no software amplifier-enable pin |

## Build and operation

Activate ESP-IDF 6.1 and run `python boards/build.py s3` from the repository root. The target uses native ESP-IDF and LVGL 9.6.0 without the Arduino framework. See [build options](../../docs/DEVELOPMENT.en.md#options) for independent module, resource-size and icon choices.

Capacitive touch does not use CYD calibration. Display submission and touch mapping are independent of resource size. Connected standby keeps networking active. Automatic deep sleep and touch/warning sounds are off by default; deep sleep is a separate optional module. Hardware power/charging indicators are not controllable RGB status LEDs.

[Installation and images](../../docs/INSTALLATION.en.md#install) · [Camera support](../CAMERA.md#en) · [Hardware](../../docs/HARDWARE.en.md)
