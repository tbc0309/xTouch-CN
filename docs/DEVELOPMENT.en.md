# Development, builds and packaging

[简体中文](DEVELOPMENT.md) | English | [Documentation](README.en.md)

CYD, S3, P4 and ES3C35P use native ESP-IDF 6.1 and LVGL 9.6.0. P4 uses the Hosted 3 host.

<a id="build"></a>

Board capability definitions are shared by the application and LVGL components. The native build audit rejects mismatched profiles before packaging.
## Building

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

Do not copy shared features into board ports. Resource size is separate from physical resolution. Use `ui/chrome_metrics.h` for shared controls and convert coordinates only once. Keep DMA buffers valid through completion; do not blindly add full framebuffers to CYD.

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

UI text uses OPPO Sans 4.0 weight 600 with Noto Sans CJK Japanese/Korean fallbacks. CYD keeps Chinese/English only. Generate fonts, icons and model images for resource profiles instead of masking wrong assets through arbitrary runtime scaling.

Language metadata is in `resources/languages.json`; do not reorder persisted indexes. Regenerate and run `scripts/check_localization.py`. Font generation requires licensed source fonts; consult generator `--help` and [third-party notices](../LICENSE-3RD-PARTY.md).

## Validation

1. Run affected protocol, authentication, isolation and UI tests; shared changes cover CYD, S3, P4 and other affected boards.
2. Build ordinary targets and run `scripts/check_native_idf.py <build-directory> --release` for dependencies, diagnostic exclusion, assets, partitions and modules. This includes test flags in IDF response files and verification that the linked portal bytes match the source and selected modules.
3. Render current production UI and inspect visible ink, alignment, clipping, scrolling and memory.
4. Check boot, retained configuration, portal, network and affected behavior on hardware; simulation is not physical evidence.
5. Before flashing, run `python scripts/check_thumbnail_upstream.py --latest-release --strict --report boards/build/thumbnail-upstream-review.json --cache boards/build/upstream-sources`. The tool requires an authenticated GitHub CLI or read-only GitHub API credentials in `GH_TOKEN`/`GITHUB_TOKEN`. Review relevant upstream changes instead of only refreshing the baseline. Maintain reusable simulator regressions for protocol changes.

Use one real-printer client per round, back up configuration, disconnect/settle between rounds and restore afterward. Separate controls from harmless queries; never automatically repeat mechanical actions. Keep unverified hardware/authentication/network items explicitly pending.


Before release, verify firmware builds, web management, provisioning, layouts and OTA slot switching. Keep hardware acceptance and private diagnostic records locally.

## Analysis tools and budgets

| Goal | Tool and limits |
|---|---|
| Flash, IRAM, DRAM and OTA capacity | Run `idf.py size`, `idf.py size-components` and `idf.py size-files` in the board project. Check the final application and partition sizes; source line count is not a capacity measurement |
| Static diagnostics | Retain GCC warnings and `compile_commands.json`. [IDF Clang-Tidy](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-clang-tidy.html) needs a matching esp-clang toolchain; use a separate build directory, without changing release compilers or automatically rewriting vendor code |
| Heap and stacks | Track current/minimum free memory, largest contiguous block and task stack headroom. Diagnostic builds may use IDF heap tracing/integrity checks; TLS needs usable contiguous memory |
| LVGL rendering | Enable SYSMON/PERF_MONITOR or profiler through the pinned LVGL Kconfig for diagnosis. Separate rendering regions/time from driver transfer time; host rendering time is not physical display FPS |
| Layout and fonts | Exercise production components in native LVGL with long names, 100 devices, languages, extreme readings, scrolling and hit tests. Inspect actual font glyphs, not just the presence of characters in strings |
| DMA and asynchronous tasks | Verify buffer ownership, completion callbacks, fallback, cancellation and exit. Observe networking and memory under pressure rather than measuring an idle bus alone |

Exclude instrumentation buffers and test endpoints from ordinary firmware. Compare the same assets, configuration and workload; disabling features, changing fonts or enlarging memory does not establish an optimization under the original conditions.

<a id="release"></a>
## Partitions and packaging

| Board | Flash | OTA application slots | Data |
|---|---|---|---|
| CYD | 4 MB | 2 × 1.875 MiB | 128 KiB |
| S3 | 16 MB | 2 × 6 MiB | 3.875 MiB |
| P4 | 32 MB | 2 × 12 MiB | 7.875 MiB |
| ES3C35P | 16 MB | 2 × 6 MiB | 3.875 MiB |

Application offsets are CYD `0x10000`, S3/P4/ES3C35P `0x20000`; merged installation starts at `0x0`. P4's bootloader is at `0x2000`. Use packaging outputs rather than mixing bootloaders/applications manually. Builds audit P4 mapping and 64-byte L2 cache-line settings.

Per-board `ota.json`, digest, length, version and Release artifacts must match. See [image names](INSTALLATION.en.md#install). Only builds that pass release checks may update public downloads and OTA manifests. Record build, simulation and physical evidence separately.

<a id="website"></a>
## Website and documentation

```sh
npm run bundle --prefix web
python boards/sync_release_metadata.py --check
python scripts/build_web.py --firmware-dir boards/build-cyd/artifacts --ota-manifest boards/build-cyd/artifacts/ota.json --board-firmware-dir boards/build-s3/artifacts --board-firmware-dir boards/build-p4/artifacts --board-firmware-dir boards/build-es3c35p/artifacts --output _site
python -m http.server 8895 --directory _site
```

The public site consumes existing artifacts; it does not replace firmware builds. Check board selection, preserved data segments, configuration capabilities and both language links for the Token and BLE Key tutorials. README contains six separate P4 screenshots in two columns; detailed examples live in the [gallery](SCREENSHOTS.en.md). Regenerate error catalogs from data, and keep internal acceptance logs outside user guides.

### Connection diagnostics

`mqttTiming.connected` is live transport state; `attemptConnected` records only the previous connection attempt. `statusReceived` distinguishes a missing initial report; `statusExpired` and `reportAgeMs` describe report freshness. An earlier successful attempt or an old report does not prove a live connection. Standalone ArduinoJson does not require the Arduino framework.

## Shared UI components

Use the shared specifications instead of choosing new values for each widget:

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

These helpers use native pixels. Convert design coordinates once at the boundary. Resource scaling selects the canvas font tier; a long action caption does not shrink that tier. Call the layout helper after changing a dynamic caption. Ordinary single-caption buttons are normalized by component creation hooks; page-specific factories use the same shared C implementation. Numeric telemetry retains its separate compact data layout.

## Regression checks

Before release, verify firmware builds, web management, provisioning, layouts and OTA slot switching. Keep hardware acceptance and private diagnostic records locally.
