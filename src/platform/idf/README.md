# ESP-IDF platform implementation

All three native board builds select `XTOUCH_NATIVE_IDF`. The component manifest, CMake dependency
checks and final link audit exclude the Arduino framework and PubSubClient.
The CYD native target is also being validated in this branch, with its own
Chinese/English resource profile and unchanged dual-OTA layout. See
`boards/esp32-2432s028r/README.md` for the migration and validation scope.

| Area | Implementation |
| --- | --- |
| Application entry | IDF `app_main`, FreeRTOS, NVS initialization |
| Cloud HTTP / thumbnails / update manifest | `esp_http_client` |
| MQTT / LAN validation | Espressif MQTT component; bounded application queue |
| Camera / FTPS / portal socket IO | `esp_tls` / lwIP / POSIX sockets |
| Wi-Fi / AP / asynchronous scan | `esp_wifi`, `esp_netif`, IDF events |
| Touch / codec control | IDF I²C master transactions, shared transaction lock |
| GPIO / backlight / timing | IDF GPIO, LEDC, `esp_timer`, FreeRTOS |
| Internal files / SD | IDF SPIFFS VFS / FAT SDMMC; no automatic card formatting |
| Vault key | Native NVS blob, existing namespace and blob format retained |
| OTA | `esp_ota_*`, ROM MD5, image validation before boot selection |
| Text / stream interfaces | Application-owned C++ utilities; no Arduino source |

Some shared application code retains familiar names such as `String`, `Stream`,
`WiFiClient` and `File`. These select the implementations in this directory on
CYD, S3 and P4. The names do not load Arduino libraries. ArduinoJson remains a standalone
C++ JSON library, built with its Arduino integrations disabled.

MQTT events only assemble bounded frames on the SDK worker. The main loop drains
the queue and invokes the existing parser/UI callbacks. Stop/destroy and profile
switching occur outside the MQTT event handler. Cloud TLS always receives its CA;
self-signed printer LAN connections preserve the existing private-address policy.

Build checks: `python scripts/check_native_idf.py boards/build-s3` (or `build-p4`).
Host regression: `python tests/idf_network/run.py --compiler g++`.
These checks do not establish physical display, TLS, SD, OTA or radio behavior;
those require separate device regression before release.

## Native validation and optimization

- `scripts/check_native_idf.py BUILD --release`: dependency, object/link and laboratory-route audit. `boards/build.py` runs it before packaging.
- `scripts/analyze_native.py BUILD --output REPORT_DIR`: installed IDF GCC `-fanalyzer` checks for selected owned C code (OTA, fault lookup and message lifetimes). The compilation database preserves the target's real defines and include paths. This is a selected-source audit, not a whole-project certification.
- `idf.py size`, `size-components`, `size-files`: compare flash and RAM by component/file before removing functionality. See [Espressif binary-size guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/size.html).
- Espressif also supplies `idf.py clang-check` / `clang-html-report` with the optional esp-clang toolchain. Check support for the selected target before changing toolchains; it is separate from the GCC release build. See [official analyzer documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32h21/api-guides/tools/idf-clang-tidy.html).
- UI regression uses actual LVGL offscreen rendering, coordinate/hit tests and allocation recovery checks. Physical panel/backlight, touch, camera, radio and power behavior still require hardware testing. LVGL's [system monitor](https://github.com/lvgl/lvgl/blob/master/docs/src/debugging/sysmon.mdx) can measure refresh/draw/flush performance; do not enable monitoring overlays in ordinary release images.

## Asset sizing

Choose small/large assets at build time independently of physical resolution. The current board defaults are CYD/S3 small, P4 large. Pre-generated bitmap fonts and static artwork avoid repeated runtime resampling at their normal size. CYD links one small printing logo and excludes model artwork; the idle xTouch logo is unchanged.

Do not rasterize every possible scaled size into flash. Larger RGB565 artwork costs two bytes per pixel (plus alpha where needed), and doubling both dimensions roughly quadruples its storage. Keep runtime fitting for variable text, extreme layouts and dynamic camera/thumbnail content. Profile draw time and temporary allocation before replacing a scalable asset with another fixed-size copy.

Native source no longer selects the former Arduino transport branches. ArduinoJson is an independent C++ dependency, not an Arduino framework dependency. OTA health confirmation remains necessary for devices retaining an older rollback-capable bootloader; removing that behavior merely because the application is native would break upgrades.

GCC documents `-fanalyzer` as suitable for C in this release. `--experimental-cpp` is a diagnostic experiment, not a C++ acceptance gate; library-model warnings must be investigated with a suitable C++ analyzer and runtime tests. See [GCC analyzer scope](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html).
