# ESP-IDF platform implementation

Native firmware uses ESP-IDF 6.1 and FreeRTOS. Framework dependencies and linked components are checked by `scripts/check_native_idf.py`.

| Area | Implementation |
|---|---|
| Application entry | IDF `app_main`, FreeRTOS and NVS |
| Cloud HTTP, thumbnails and update manifests | `esp_http_client` |
| MQTT and LAN validation | Espressif MQTT with a bounded application queue |
| Camera, FTPS and portal socket I/O | `esp_tls`, lwIP and POSIX sockets |
| Wi-Fi, access point and asynchronous scanning | `esp_wifi`, `esp_netif` and IDF events |
| Touch and codec control | IDF I²C master with shared transaction locking |
| GPIO, backlight and timing | IDF GPIO, LEDC, `esp_timer` and FreeRTOS |
| Internal storage and SD | SPIFFS VFS and FAT SDMMC; cards are not formatted automatically |
| Credential storage | Device-bound keys in NVS |
| OTA | `esp_ota_*` with image validation before boot selection |

Application utilities such as `String`, `Stream`, `WiFiClient` and `File` are implemented in this directory. They do not link the Arduino framework. ArduinoJson is an independent C++ library with Arduino integrations disabled.

MQTT callbacks assemble bounded frames on the SDK worker; the main loop processes status and UI updates. Connection destruction and printer switching occur outside MQTT event callbacks. Cloud TLS validates its CA; printer LAN TLS uses the private-address access policy.

## Build diagnostics

- `python scripts/check_native_idf.py BUILD --release`: framework, linked components, modules, resources and image checks.
- `python scripts/analyze_native.py BUILD --output REPORT_DIR`: GCC analysis of selected application-owned C sources, using the build's compilation database.
- `idf.py size`, `idf.py size-components`, `idf.py size-files`: application and component memory usage.
- IDF Clang-Tidy requires the matching optional esp-clang toolchain. See [Espressif documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-clang-tidy.html).

## Display resources

Resource size is independent of physical resolution. CYD and Waveshare S3 use small resources by default; P4 and ES3C35P use large resources. Fonts, icons and static model artwork are generated at build time. CYD uses one printing logo and keeps the idle xTouch logo.

Dynamic text and camera frames retain runtime fitting. Resource profiles, font options and board adapters are described in the [development guide](../../../docs/DEVELOPMENT.en.md).
