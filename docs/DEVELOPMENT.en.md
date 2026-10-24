# Development, builds and releases

[简体中文](DEVELOPMENT.md) | English | [Documentation](README.en.md)

[Build](#build) · [Source layout](#architecture) · [Build options](#options) · [Packaging and OTA](#release) · [Installer website](#website)

<a id="build"></a>

## Build

Normal builds use the generated fonts and graphics in the repository; original font files are not required.

### ESP32-2432S028R

Requires Python, Node.js and PlatformIO:

```sh
npm ci --prefix web
python -m pip install platformio==6.1.19 -r scripts/requirements.txt
python -m platformio run -e esp32dev
```

The application is `.pio/build/esp32dev/firmware.bin`. Packages are written to `xtouch-bin/ota/` and `xtouch-bin/webusb/` beside the repository directory.

### Waveshare S3 V2 / P4

Install and activate ESP-IDF 6.1, then run from the repository root:

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
python scripts/embed_portal.py
python boards/build.py s3
python boards/build.py p4
```

Artifacts are written to `boards/build-s3/artifacts/` and `boards/build-p4/artifacts/`. Dependency versions are recorded in `boards/dependencies.json` and component manifests. Initial builds require network access.

Run **Build firmware** manually in GitHub Actions to build all three boards. Packages appear in that run's Artifacts and do not automatically replace Release files.

### Fonts and translations

The interface uses OPPO Sans 4.0 at weight 600, with missing Japanese kana and Korean glyphs from Noto Sans CJK. Small and large screens have separate native pixel sizes. See [third-party licenses](../LICENSE-3RD-PARTY.md#oppo-sans-40).

Translations are in `src/ui/i18n/`, `resources/ui-locales.json` and `resources/portal-locales.json`. Font regeneration requires authorized source fonts:

```sh
python -m pip install fonttools
python scripts/generate_text_fonts.py --font /path/to/font.ttf --weight 600 --fallback-font /path/to/NotoSansCJK-Regular.otf
python scripts/check_localization.py
```

`scripts/generate_ui_assets.py` generates icons and logos; `scripts/generate_printer_images.py` generates printer artwork. Resource sources and licenses are in the respective NOTICE files.

<a id="architecture"></a>

## Source layout

| Directory | Contents |
|---|---|
| `src/ui/` | Pages, dialogs, layouts, text fitting and translations |
| `src/xtouch/` | MQTT, printer state, accounts, configuration and portal APIs |
| `src/features/` | Power, sound, camera, thumbnails and other modules |
| `boards/common/` | Hardware interfaces, display adaptation and shared driver support |
| `boards/<board>/` | Pins, display, touch, audio and SDK configuration |
| `portal/` | Device management portal |
| `web/` | Online installer and error lookup website |
| `resources/` | Dictionaries, font configuration and graphics |

CYD uses PlatformIO/Arduino; S3/P4 use ESP-IDF. Pages and printer features share the same source, with board-specific drivers and resource sizes.

### Shared UI and resources

All three boards share components, layout, language refresh and interaction in `src/ui/`, adapting to display size and hardware capabilities. Native pixel fonts live in `src/ui/fonts/` and `resources/ui-assets/fonts/{small,large}/`; generators and native previews use the same assets. Board directories contain hardware drivers and configuration. Persistent dialogs refresh after loading/changing the display language and update action captions when shown.

Language metadata lives in `resources/languages.json`; `scripts/generate_locales.py` generates the device and web lists. Existing language order maps to persisted indices and must remain stable. Device settings use independent `language` and `portalLanguage` values for the display and portal; both default to `en`. Initial Wi-Fi setup initializes both values; they can then be changed independently.

<a id="options"></a>

## Build options

S3/P4 accept CMake `-DNAME=ON/OFF` options:

| Option | Default | Function |
|---|---|---|
| `XTOUCH_FEATURE_CLOUD_TOKEN` | ON | Access Token entry and validation |
| `XTOUCH_FEATURE_CLOUD_LOGIN` | ON | Account password and verification-code sign-in |
| `XTOUCH_FEATURE_AUDIO` | ON | Audio driver and sounds |
| `XTOUCH_FEATURE_DEEP_SLEEP` | ON | Optional idle deep sleep |
| `XTOUCH_FEATURE_INDICATOR` | ON | Software-controllable indicators |
| `XTOUCH_FEATURE_CAMERA` | ON | Camera viewing |
| `XTOUCH_FEATURE_PRINT_PREVIEW` | ON | Job thumbnails |
| `XTOUCH_FEATURE_BLE_PROVISIONING` | OFF | S3/P4 BLE QR provisioning |
| `XTOUCH_ERROR_DETAILS` | OFF | Chinese/English fault descriptions and extended glyphs |
| `XTOUCH_ICON_SET` | Automatic | `original` for small screens, `mdi` for large screens |

BLE provisioning also requires `CONFIG_BT_ENABLED`, `CONFIG_BT_NIMBLE_ENABLED` and `CONFIG_ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1` in sdkconfig. P4 additionally needs `CONFIG_ESP_HOSTED_HOST_FEAT_BT`. This board defaults to its preinitialized C6 Bluetooth controller (`XTOUCH_P4_C6_PREINITIALIZED_BT=ON`). Set it to OFF only after installing C6 firmware that supports controller-management RPC.

Build with `python boards/build.py s3 --ble-provisioning` or `python boards/build.py p4 --ble-provisioning`. With Hosted 3, P4 BLE provisioning requires `--c6-managed-bt` and compatible controller-management RPC firmware. The stock preinitialized C6 is supported for thermometer reception, but not provisioning teardown; the build rejects that combination. BLE builds use a separate directory and sdkconfig, leaving normal builds unchanged. The CYD BLE provisioning build exceeds the dual-OTA application capacity; release images retain dual OTA slots.

Example: `python boards/build.py s3 --without audio camera print_preview --icons original`. CYD defaults are in `src/features/config.h`; the audio driver is enabled by default, while camera and thumbnails are disabled. Select resource size with `--assets small|large` (`XTOUCH_ASSET_SIZE` for CYD) and icon style with `--icons original|mdi`. Current defaults are small/original for CYD/S3 and large/MDI for P4. CYD validates linked metrics and glyph pixels against the selected resource profile before packaging.

The portal displays settings included in the firmware and supported by its hardware. See [Board documentation](../boards/README.md) for display and driver settings.

### Memory and storage

The P4 32 MB Flash configuration enables ESP-IDF 32-bit address cache mapping for both 12 MiB OTA slots. Build the bootloader and application with the same configuration; bootloader updates require a full USB flash.

CYD has no PSRAM; S3 has 8 MB and P4 has 32 MB. S3/P4 use PSRAM for image and decoder data, retaining internal memory for display DMA and task stacks. Fullscreen camera viewing pauses thumbnail work and some background checks; MQTT and touch-to-exit remain active.

MQTT connection setup uses synchronous network calls and may briefly affect UI response. CYD releases printer TLS while reading configuration to reclaim memory; S3/P4 retain it by default.

<a id="release"></a>

## Packaging and OTA

| Board | Application | Full USB image | Application offset |
|---|---|---|---|
| ESP32-2432S028R | `xtouch-2.8.bin` | `xtouch-2.8-full.bin` | `0x10000` |
| ESP32-S3-Touch-LCD-2.8 V2 | `esp32-s3-touch-lcd-2.8-v2.bin` | `esp32-s3-touch-lcd-2.8-v2-full.bin` | `0x20000` |
| ESP32-P4-WIFI6-Touch-LCD-4.3 | `esp32-p4-wifi6-touch-lcd-4.3.bin` | `esp32-p4-wifi6-touch-lcd-4.3-full.bin` | `0x20000` |

Full images are written at `0x0`; within them, the P4 bootloader starts at `0x2000` and S3 at `0x0`. Application images are used for portal, OTA and SD updates.

| Board directory | Flash | OTA application slots | Data partition |
|---|---|---|---|
| `esp32-2432s028r` | 4 MB | 2 × 1.875 MiB | 128 KiB |
| `esp32-s3-touch-lcd-2.8-v2` | 16 MB | 2 × 6 MiB | 3.875 MiB |
| `esp32-p4-wifi6-touch-lcd-4.3` | 32 MB | 2 × 12 MiB | 7.875 MiB |

OTA manifests are `boards/<board-directory>/ota.json`. S3/P4 package metadata is stored alongside them as `manifest.json`. Versions, lengths and digests must match Release images. Packaging checks chip identity, application size, merged offsets and partition bounds.

Release downloads provide the six BIN files listed above. Package metadata is used for builds and verification. See [Installation](INSTALLATION.en.md#install) for updates, backups and recovery.

<a id="website"></a>

## Installer website

The **Configuration and web installer** Action builds and deploys GitHub Pages using existing Release images; it does not compile firmware.

Local builds need matching CYD Release images and optional S3/P4 package directories:

```sh
npm ci --prefix web
npm run bundle --prefix web
python scripts/build_web.py --firmware-dir release-images --board-firmware-dir boards/build-s3/artifacts --board-firmware-dir boards/build-p4/artifacts --output _site
python -m http.server 8895 --directory _site
```

Open `http://localhost:8895/`. The build validates images and manifests and generates installation segments that preserve user-data partitions. The flasher is bundled from source modules; dependency and P4 downloader licenses are listed in `web/flasher/NOTICE.md`.

## Optional Xiaomi BLE receiver

Build with `python boards/build.py s3 --xiaomi-ble` or `python boards/build.py p4 --xiaomi-ble`. P4 defaults to a preinitialized C6 controller; use `--c6-managed-bt` after installing C6 firmware supporting controller-management RPC. This uses an independent build directory and sdkconfig. The receiver and BLE provisioning are mutually exclusive. S3/P4 release builds include the receiver; BLE provisioning remains disabled by default. These modules exceed CYD dual-OTA application capacity. See [thermometers](THERMOMETERS.en.md) for models, binding and reporting.

Use `--without xiaomi_ble` to omit the thermometer receiver.

P4 application and bootloader builds use 64-byte L2 cache lines. Configuration and packaging reject stale settings.

## Cloud authentication modules

`XTOUCH_FEATURE_CLOUD_TOKEN` controls Access Token entry/validation. `XTOUCH_FEATURE_CLOUD_LOGIN` controls account password, verification-code authentication and saved-password reauthentication. Both default ON for S3/P4; CYD defaults to Token ON and account login OFF. Generated browser assets include only selected authentication modules. Shared authenticated sessions, bound-device queries and MQTT still use tokens regardless of the selected entry method.

Use `python boards/build.py s3 --without cloud_login` or `--without cloud_token` (also supported for P4). CYD PlatformIO builds can override the macros with build flags. Backend action validation follows the same build switches; disabling an entry is not only a browser visibility setting.

## HTTP transport and protocol compatibility

S3/P4 use ESP-IDF `esp_http_client` for account APIs, task/thumbnail APIs, image and OTA downloads, and simulator discovery. Streaming readers retain feature-specific size/time limits. Covers require trusted CDN hosts and never receive account authorization headers. Authenticated requests do not follow redirects. CYD retains its Arduino HTTP transport to fit the existing dual-OTA layout. MQTT continues to use its separate PubSubClient/TLS transport.

Telemetry accepts legacy temperatures and packed bed fields. Airduct fans are mapped by reported type, function, ID and range; doors are not treated as fans. Bed commands follow the `fun` capability flag, and chamber lights are selected by node name. Command replies are correlated by sequence ID and command name; status reports and errors belonging to other clients are not treated as acknowledgements for this display.

### P4 host communication stack

P4 uses ESP-Hosted 3.0.10 and retains the existing C6 firmware. `boards/prepare.py` fetches the pinned Arduino 3.3.12 source and applies the Hosted dependency constraint required by this adapter; no global SDK files are changed. Host initialization and Arduino Kconfig compatibility aliases are part of the board port. Updating P4 does not write C6 firmware.
