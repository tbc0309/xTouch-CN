# CYD native ESP-IDF port

ESP32-2432S028R uses ESP-IDF 6.1 and LVGL 9.6.0, 4 MB Flash and no PSRAM.

The profile retains dual OTA, Chinese/English text, small original icons, the idle xTouch logo and green Bambu printing artwork. Device selection is list-only. Thermometer, Token and password/code account modules are included. Camera, task thumbnails and full error descriptions are omitted for memory/capacity.

Display and XPT2046 touch use native drivers; the Arduino framework is not linked. Touch calibration is device-specific. The UI reserves a 48 KiB graphics pool and one 16-row RGB565 DMA buffer (10 KiB) in internal RAM. The cloud worker stack is 12 KiB. The buffer budget and region checks are shared with the host display regression; invalid or oversized regions are rejected before DMA submission. Firmware capacity is checked against each 0x1e0000-byte OTA slot. Do not erase configuration merely to migrate frameworks.

See [hardware pins](../../docs/HARDWARE.en.md#cyd), [build instructions](../../docs/DEVELOPMENT.en.md#build), [resource rules](../../docs/DEVELOPMENT.en.md) and [installation](../../docs/INSTALLATION.en.md#install).

The printing logo identifies the printer brand and does not imply affiliation. Asset licensing is listed in [third-party notices](../../LICENSE-3RD-PARTY.md).
