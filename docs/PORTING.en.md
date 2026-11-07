# Board porting interfaces

Keep board adapters in `boards/<directory>/` and shared protocol, UI and storage lifetime code in `src/`. Do not copy application modules or impersonate another board macro to inherit its pins. A build identity is not proof of hardware support.

## Identity and build

[board_catalog.py](../boards/board_catalog.py) is the shared identity source. Stable boards retain their existing directories, asset names and partitions. Optional `boards/port_targets.json` and `boards/bsp_targets.json` extend the catalog. Each entry needs unique target/directory/stem and a unique project within its chip family, plus `chip_id`, `flash`, `screen` and the existing packaging/display fields. Extensions default to the `test` release channel. Capability flags are booleans; build and physical acceptance statuses are separate.

```sh
python boards/build.py s3 --dry-run
python boards/build.py s3 --jobs 4
python boards/build.py s3 --without audio camera --jobs 4
python boards/build.py s3 --ble-provisioning
```

Dry-run prints the selected resources/modules without writing configuration or invoking IDF. Jobs sets compiler concurrency. Thermometer reception and BLE provisioning remain mutually exclusive. Resource size can be explicitly selected with `--assets small` or `--assets large`.

The board root project must include `boards/common/idf_project.cmake` before `project(...)`. This initializes IDF and exports module selections into its isolated requirements scan. Bypassing it with the SDK project include can produce dependencies inconsistent with disabled application modules. SDK defaults remain board-owned.

## Adapter contract

| File | Responsibility |
| --- | --- |
| `CMakeLists.txt`, `sdkconfig.defaults` | Actual SoC, memory, SDK capabilities and module defaults. |
| `partitions.csv` | Dual OTA and data layout; limits derive from equal application slots, never guessed from Flash size. |
| `display.cpp` | Display, touch, brightness and sleep interfaces from [board.h](../boards/common/board.h). |
| Optional `storage.cpp` | [storage_profile.h](../boards/common/storage_profile.h) SPI/SDMMC wiring and board power. Missing adapters mean no card support. |
| Optional `power.h` | Clock policy, deep-sleep preparation/restoration and touch wake. Missing adapters do not manipulate GPIOs or enable deep touch wake. |
| Optional `network.h` | `board_network_begin()` and coprocessor power/transport initialization. |
| Optional `audio.cpp`, `indicator.cpp` | Hardware drivers. Disable unavailable modules. |
| Optional `board_sources.cmake` | Set `BOARD_DISPLAY_SOURCE`; append board driver files/definitions/components to `BOARD_EXTRA_SOURCES`, `BOARD_EXTRA_DEFINITIONS`, `BOARD_PRIVATE_DEFINITIONS`, `BOARD_REQUIRES`, `AUDIO_REQUIRES`. |

BSP integrations follow the same boundary. `XTOUCH_BOARD_POWER_HEADER` and `XTOUCH_BOARD_NETWORK_HEADER` can select alternate adapters implementing the same interfaces. Declare feature defaults before entering the shared component, rather than scattering product-name conditions across application code.

Shared code owns card mounting, retirement, file leases and delayed unmount. A failed adapter configuration or mount calls `release`; it must be idempotent. Never release a card or power while a file lease remains open. The P4 SD/SDIO interrupt correction is shared and does not modify SDK source files.

## Modules and OTA

H.264 dependencies are selected only for an enabled camera on a supported SoC. Other camera targets may keep JPEG and report an unsupported format. BLE provisioning is explicitly selected as well. Optional managed packages use `require: no` with explicit application dependencies. The component manager may still build downloaded static libraries; their presence alone does not enable a module or show that its implementation is linked into the final ELF.

[ota_channels.json](../resources/ota_channels.json) defines the repository and channels. Stable firmware uses `main` only; test firmware uses `main` and `test` with a matching `-test` version. Forks can change the repository identity; generation and packaging use it consistently. Generated board identity, HTTP limit, filename and partition capacity must agree. Unknown targets fail instead of inheriting CYD identity, and stable firmware rejects test manifests.

## Acceptance

Verify manufacturer wiring, memory, panel timing and touch coordinates. Run `python boards/check_contracts.py`, check dry-run selection, then fully build and run `scripts/check_native_idf.py <build-directory> --release`. Review ELF/image identity, resource profile, module exclusion, OTA channel and matching application/LVGL definitions. With hardware, separately test display, touch, audio, indicator, networking, card failures/removal, standby/wake and retained settings. Mark unavailable physical checks honestly.

Build tools do not change global Git configuration. Git child processes disable automatic CRLF conversion to preserve pinned source hashes.
