# Development, builds and packaging

[简体中文](DEVELOPMENT.md) | English | [Documentation](README.en.md)

CYD, S3, P4 and ES3C35P use native ESP-IDF 6.1 and LVGL 9.6.0. P4 uses the Hosted 3 host.

<a id="build"></a>

Board capability definitions are shared by the application and LVGL components. The native build audit rejects mismatched profiles before packaging.
## Building

Build CYD for its actual controller: `python boards/build.py cyd` selects ILI9341 with the existing ILI9341_2 parameters; `python boards/build.py cyd-st7789` selects ST7789. Both share native hardware drivers, but use separate project identities, artifact names and OTA directories to prevent cross-panel updates. USB connector count alone is not reliable identification. ST7789 retains 40 MHz SPI, the 16-row DMA buffer and touch calibration.

Activate ESP-IDF 6.1 and run from the repository root:

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
python boards/build.py cyd
python boards/build.py s3
python boards/build.py p4
python boards/build.py es3c35p
```

The entry point regenerates the matching portal language variants from readable sources, prepares dependencies, configures modules, audits native dependencies and packages artifacts under `boards/build-<board>/artifacts/`. Initial dependency resolution needs networking. Versions are pinned in `boards/dependencies.json`, lockfiles and component manifests. Normal builds use generated resources without source TTF files.

Build [LCDWIKI ES3C35P](../boards/lcdwiki-es3c35p/README.en.md) with `python boards/build.py es3c35p`. Its project identity and image are separate from Waveshare S3; matching chips do not make the images interchangeable.

<a id="architecture"></a>
## Code organization

The main loop and LVGL run on CPU1; camera decoding runs on CPU0. Workers exchange data with the UI instead of calling LVGL. Three separate camera canvases exchange ownership under a mutex. The decoder acquires a writable canvas for each frame, avoiding a full-frame copy or overwriting the displayed image. Release audits verify task affinity in the generated configuration.

| Directory | Responsibility |
|---|---|
| `src/ui/` | Shared pages, metrics, semantic colors, fonts and interaction |
| `src/features/` | Optional features, protocol policy and diagnostics |
| `src/platform/idf/` | Application-owned native network/storage/runtime tools |
| `src/xtouch/` | Status, configuration, account and route integration |
| `boards/common/` | Hardware interfaces and lifecycle |
| `boards/<board>/` | Pins, panel, touch, audio and SDK defaults |
| `portal/` / `web/` | Device management / public installer and lookup |
| `resources/` / `scripts/` | Inputs, licenses, generation and audits |

Board ports use shared features and controls. Resource size is independent of physical resolution; `ui/chrome_metrics.h` supplies shared metrics. DMA retains ownership of a submitted buffer until transfer completion.

<a id="options"></a>
## Modules and resources

```sh
python boards/build.py s3 --without audio --assets small --icons original
```

| Module | Build-entry default |
|---|---|
| `cloud_token` / `cloud_login` | ON, independent Token and password/code modules |
| `xiaomi_ble` | ON, thermometer reception |
| External chamber `--external-chamber` | OFF; independent CYD GPIO22 DS18B20 adapter |
| `audio` / `deep_sleep` / `indicator` | ON, subject to runtime preferences and hardware |
| `camera` / `print_preview` | CYD OFF, S3/P4/ES3C35P ON |
| Bluetooth provisioning `--ble-provisioning` | OFF; ESP32-S3 boards only; mutually exclusive with thermometer builds |

The optional wired adapter uses `settings.chamberTemp` to enable readings and `settings.chamberTempDiff` for calibration. These parameters cannot enable an excluded module. The screen's Thermometer dropdown edits the separate `environmentSensor.display` preference.

`--without` removes code, not just UI. CYD/S3 default to small original resources; P4/ES3C35P to large MDI. Select independently with `--assets small|large` and `--icons original|mdi`. CYD uses Chinese/English, single printing artwork and list-only selection; S3/P4/ES3C35P retain multilingual/image resources.

Each combination must pass partition, font and runtime-memory checks. In particular, not every combination necessarily fits CYD; do not bypass capacity checks.


### Fonts and generated assets

The CMake option `XTOUCH_FILENAME_FONT=auto|common-zh|ui` selects filename coverage. Independent punctuation supplementation uses `XTOUCH_FILENAME_PUNCTUATION`, default `ON`, covering 99 common punctuation marks and symbols. Selecting `ui` disables common Han expansion while retaining punctuation and all fixed UI captions. `auto` uses the MCU target: the file-browser module on ESP32-S3 and ESP32-P4 includes the 6,763 common GB2312 Han characters plus GB2312 symbols and common typographic punctuation by default; other MCUs retain UI-only coverage. Board names and asset size do not determine this policy. Builds without the file-browser module do not link these fonts, and choosing a font profile does not enable that module. The supplement stores only glyphs missing from the matching UI font size; existing UI glyphs are reused through a fallback chain. Shared assets are in `resources/fonts/file-names/`; regenerate them with `scripts/generate_filename_fonts.py`. UI font generation automatically adds or removes supplemental glyphs. Builds validate `coverage.json` and reject outdated coverage. The 28px font uses a static fallback chain to keep each bitmap within the compact descriptor index limit, supporting small and large asset profiles without changing the global LVGL font layout.

UI text uses OPPO Sans 4.0 weight 600 with Noto Sans CJK Japanese/Korean fallbacks. CYD keeps Chinese/English only. Generate fonts, icons and model images for resource profiles instead of masking wrong assets through arbitrary runtime scaling.

Language metadata is in `resources/languages.json`; do not reorder persisted indexes. Regenerate and run `scripts/check_localization.py`. Font generation requires licensed source fonts; consult generator `--help` and [third-party notices](../LICENSE-3RD-PARTY.md).

## Build checks

```sh
python scripts/check_localization.py
python scripts/check_documentation.py
python boards/check_contracts.py
python scripts/check_native_idf.py boards/build-s3 --release
```

The native audit checks framework dependencies, module selection, resources, image identity and partition capacity. Use the build directory for the selected target.

## Diagnostic tools

| Area | Tool |
|---|---|
| Flash and RAM usage | `idf.py size`, `idf.py size-components`, `idf.py size-files` |
| Static analysis | `compile_commands.json` and [IDF Clang-Tidy](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-clang-tidy.html), with a matching esp-clang toolchain |
| Heap and task stacks | IDF heap tracing, integrity checks and task stack high-water marks |
| LVGL performance | LVGL Kconfig SYSMON/PERF_MONITOR or profiler |

Diagnostic options affect memory usage and rendering performance. Ordinary firmware excludes test endpoints.

<a id="release"></a>
## Partitions and packaging

| Board | Flash | OTA application slots | Data |
|---|---|---|---|
| CYD | 4 MB | 2 × 1.875 MiB | 128 KiB |
| S3 | 16 MB | 2 × 6 MiB | 3.875 MiB |
| P4 | 32 MB | 2 × 12 MiB | 7.875 MiB |
| ES3C35P | 16 MB | 2 × 6 MiB | 3.875 MiB |

Application offsets are CYD `0x10000`, S3/P4/ES3C35P `0x20000`; merged installation starts at `0x0`. P4's bootloader is at `0x2000`. Use packaging outputs rather than mixing bootloaders/applications manually. Builds audit P4 mapping and 64-byte L2 cache-line settings.

Packaging writes matching version, size and checksum records to each board’s `ota.json` and manifest. See [image names](INSTALLATION.en.md#install).

<a id="website"></a>
## Website and documentation

```sh
npm run bundle --prefix web
python boards/sync_release_metadata.py --check
python scripts/build_web.py --firmware-dir boards/build-cyd/artifacts --ota-manifest boards/build-cyd/artifacts/ota.json --board-firmware-dir boards/build-s3/artifacts --board-firmware-dir boards/build-p4/artifacts --board-firmware-dir boards/build-es3c35p/artifacts --output _site
python -m http.server 8895 --directory _site
```

The website is built from existing firmware artifacts and matching manifests. Screenshots are in the [gallery](SCREENSHOTS.en.md); error-code pages are generated from the source database.

### Connection diagnostics

`mqttTiming.connected` is live transport state; `attemptConnected` records only the previous connection attempt. `statusReceived` distinguishes a missing initial report; `statusExpired` and `reportAgeMs` describe report freshness. An earlier successful attempt or an old report does not prove a live connection. Standalone ArduinoJson does not require the Arduino framework.

## Shared UI components

Shared UI metrics, colours and fonts are available through these interfaces:

| Concern | Entry point |
|---|---|
| Spacing, insets, corners and control sizes | `src/ui/ui_specs.h`: `ui_specs_get()` |
| Titles, close controls and toolbars | `src/ui/chrome_metrics.h` |
| Semantic colors and states | `src/ui/palette.h` |
| Font resources and size tiers | `src/ui/native_fonts.h`, `src/ui/text_fit.h` |
| Icon family and resource selection | `src/ui/symbol_fonts.h`, `resources/ui-assets/` |
| Icon plus caption | `ui_layout_action_fit()`; fixed icon, text-only overflow scrolling |
| Dynamic-page button captions | `ui_layout_setup_caption()`; shared C layout maintains centering after resizing |
| Setup choices and QR codes | `ui_layout_setup_choice_content()` / `ui_layout_qr_size()`; native geometry, fixed icons and independent text scrolling |
| Material card content | `ui_specs_material_content()`; swatch, balanced reading/material rows and bottom inset |

These helpers use native pixels. Convert design coordinates once at the boundary. Resource scaling selects the canvas font tier; long action captions scroll at that font size. Call the layout helper after changing a dynamic caption. Ordinary single-caption buttons are normalized by component creation hooks; page-specific factories use the same shared C implementation. Numeric telemetry retains its separate compact data layout.

[Board adapter interfaces](PORTING.en.md)

### File browser and pixel-display modules

Standard ESP32-S3 / ESP32-P4 builds enable both modules. CYD omits them because of storage capacity. Disable with `--without gcode_browser pixel_screen`, or explicitly enable with `--gcode-browser --pixel-screen`. File browsing requires an available SD card. Pixel linking and BLE provisioning are mutually exclusive; the thermometer and pixel display share the BLE lifecycle and host leases. BLE-provisioning builds default to pixel linking OFF. Direct CMake builds use `XTOUCH_FEATURE_GCODE_BROWSER` and `XTOUCH_FEATURE_PIXEL_SCREEN`; `boards/common/interactive_modules.cmake` selects the defaults before Bluetooth SDK settings.

The file browser is enabled by default only when the board has a mapped `storage.cpp` adapter. Targets without SD support omit the file browser and its filename-font consumers; pixel-screen selection is independent.
