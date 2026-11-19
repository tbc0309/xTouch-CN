# Hardware initialization and component notices

CO5300 glass initialization, offsets and the CST9217 orientation follow the
Waveshare ESP32-S3-Touch-AMOLED-1.75 BSP, version 3.0.1, at vendor repository
commit `e4344e70c2fa78a13e8a06566507f1ba8af6672a`:

https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75

Vendor BSP copyright: 2025 Waveshare; SPDX-License-Identifier: Apache-2.0.
Only panel command data and the documented hardware configuration are reused;
the BSP's application and LVGL adapter are not linked into this target.

The IDF component manager resolves the pinned CO5300, CST9217 and ES8311
components declared in `xtouch_board_support/idf_component.yml`. Their source
files retain their respective copyright and license notices. The complete
resolved versions and hashes are recorded in `dependencies.lock`.
