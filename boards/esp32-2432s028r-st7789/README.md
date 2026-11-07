# CYD ST7789 firmware

Native ESP-IDF profile for the 2.8-inch 240×320 CYD with ST7789/ST7789V and XPT2046 resistive touch. Confirm the actual controller; USB connector count alone is not sufficient identification.

Build with `python boards/build.py cyd-st7789`. Application identity `xtouch_cyd_st7789`, artifact `xtouch-2.8-st7789.bin` and OTA manifest directory are separate from the ILI9341 firmware. OTA and application upload reject a different project identity. Select the matching full image for the initial USB installation.

Display, DMA, touch, storage, audio and power implementations are shared with the ILI9341 CYD adapter. SPI remains 40 MHz with the existing 16-row buffer. ST7789 uses its own initialization, landscape RGB order and default inversion; user inversion remains relative to normal colors. Flash, dual OTA layout and module defaults match the original CYD.
