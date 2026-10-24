ESP32-P4 revision 3+ flasher stub: Espressif esp-flasher-stub v1.4.1,
https://github.com/espressif/esp-flasher-stub/releases/tag/v1.4.1

The JSON binary is distributed under Apache-2.0 OR MIT; both license texts
are included here. It is the same revision-3+ stub shipped with esptool 5.6.0.
The website retains the esptool-js 0.5 API required by ESP Web Tools 10.1.0.
Build patches add P4 security-info detection, revision decoding, ECO6/ECO7
Flash power sequencing, extended download commands and 32 MB Flash setup.
ESP32 ROMs without security-info support retain legacy chip detection.
Trace formatting is skipped when tracing is disabled.

The installer is bundled from ESP Web Tools source modules, not its prebuilt
browser distribution. Material Web is pinned to its compatible 2.2.0 API.
SparkMD5 3.0.2 verifies each written segment against the device's MD5 result;
its alternative MIT license is included as LICENSE.spark-md5.
Source: https://github.com/satazor/js-spark-md5/blob/master/LICENSE2

P4 revision decoding and ECO6/ECO7 register setup are adapted from
Espressif esptool 5.6.0, esptool/targets/esp32p4.py (GPL-2.0-or-later).
Copyright 2024-2025 Fredrik Ahlberg, Angus Gratton, Espressif Systems
(Shanghai) CO LTD and other contributors. See LICENSE.esptool.
The adaptation is distributed under this project's GPLv3 license.
Source: https://github.com/espressif/esptool/blob/master/esptool/targets/esp32p4.py
