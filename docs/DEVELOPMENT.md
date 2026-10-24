# 开发、构建与发布

简体中文 | [English](DEVELOPMENT.en.md) | [文档目录](README.md)

[构建](#build) · [代码结构](#architecture) · [编译选项](#options) · [打包与 OTA](#release) · [刷机页](#website)

<a id="build"></a>

## 构建

普通编译使用仓库中的已生成字体和图形资源，无需原始字体文件。

### ESP32-2432S028R

需要 Python、Node.js 和 PlatformIO：

```sh
npm ci --prefix web
python -m pip install platformio==6.1.19 -r scripts/requirements.txt
python -m platformio run -e esp32dev
```

应用镜像位于 `.pio/build/esp32dev/firmware.bin`；打包结果位于仓库相邻的 `xtouch-bin/ota/` 和 `xtouch-bin/webusb/`。

### Waveshare S3 V2 / P4

安装并激活 ESP-IDF 6.1，然后在仓库根目录执行：

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
python scripts/embed_portal.py
python boards/build.py s3
python boards/build.py p4
```

对应产物位于 `boards/build-s3/artifacts/` 和 `boards/build-p4/artifacts/`。依赖版本记录于 `boards/dependencies.json` 和组件依赖清单，首次构建需要联网。

GitHub Actions 的 **Build firmware** 可手动编译并打包三板，产物保存在该次运行的 Artifacts 中，不会自动替换 Release 文件。

### 字体与多语言资源

界面使用 OPPO Sans 4.0，字重 600；日文假名和韩文缺字由 Noto Sans CJK 补齐。大小屏具有独立的原生像素字号。字体授权见[第三方许可](../LICENSE-3RD-PARTY.md#oppo-sans-40)。

翻译位于 `src/ui/i18n/`、`resources/ui-locales.json` 和 `resources/portal-locales.json`。重新生成字体需要合法授权的源字体：

```sh
python -m pip install fonttools
python scripts/generate_text_fonts.py --font /path/to/font.ttf --weight 600 --fallback-font /path/to/NotoSansCJK-Regular.otf
python scripts/check_localization.py
```

`scripts/generate_ui_assets.py` 生成图标与 Logo；`scripts/generate_printer_images.py` 生成机型图。资源来源与许可保存在相应目录的 NOTICE 中。

<a id="architecture"></a>

## 代码结构

| 目录 | 内容 |
|---|---|
| `src/ui/` | 页面、弹窗、布局、文字适配和多语言 |
| `src/xtouch/` | MQTT、打印机状态、账号、配置及网页接口 |
| `src/features/` | 电源、声音、摄像头、缩略图及其他功能模块 |
| `boards/common/` | 硬件接口、显示适配及共用驱动支持 |
| `boards/<主板>/` | 主板引脚、显示、触摸、音频及 SDK 配置 |
| `portal/` | 设备网页后台 |
| `web/` | 在线安装与故障码查询网站 |
| `resources/` | 字典、字体配置及图像资源 |

CYD 使用 PlatformIO/Arduino，S3/P4 使用 ESP-IDF。页面和打印机功能共用；主板层提供分辨率、资源档位及硬件能力。

### 共享界面与资源

三种主板共用 `src/ui/` 的组件、布局、语言刷新和交互实现，按显示尺寸及硬件能力适配。通用像素字库位于 `src/ui/fonts/` 和 `resources/ui-assets/fonts/{small,large}/`，生成器与原生预览使用相同资源；板级目录保留驱动和硬件配置。常驻弹窗在加载或切换屏幕语言时刷新，显示时也更新按钮文字。

语言元数据统一维护于 `resources/languages.json`，由 `scripts/generate_locales.py` 生成设备端和网页端列表；已有语言顺序对应持久化索引，不应重排。设备配置中的 `language` 与 `portalLanguage` 分别用于屏幕和后台，缺省均为 `en`。首次 Wi-Fi 配网所选语言同时初始化两者，之后可以独立更改。

<a id="options"></a>

## 编译选项

S3/P4 可通过 CMake `-D名称=ON/OFF` 选择模块：

| 选项 | 默认 | 功能 |
|---|---|---|
| `XTOUCH_FEATURE_AUDIO` | ON | 音频驱动和提示音 |
| `XTOUCH_FEATURE_DEEP_SLEEP` | ON | 可选空闲深睡 |
| `XTOUCH_FEATURE_INDICATOR` | ON | 可软件控制的指示灯 |
| `XTOUCH_FEATURE_CAMERA` | ON | 摄像头 |
| `XTOUCH_FEATURE_PRINT_PREVIEW` | ON | 任务缩略图 |
| `XTOUCH_FEATURE_BLE_PROVISIONING` | OFF | S3／P4 蓝牙扫码配网 |
| `XTOUCH_ERROR_DETAILS` | OFF | 中英文故障说明与扩展字形 |
| `XTOUCH_ICON_SET` | 自动 | 小屏 `original`，大屏 `mdi` |

启用蓝牙配网还需在 sdkconfig 开启 `CONFIG_BT_ENABLED`、`CONFIG_BT_NIMBLE_ENABLED`、`CONFIG_ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1`。P4 需开启 ESP-Hosted NimBLE／VHCI；仅对已在启动时初始化蓝牙控制器的 C6 固件使用 `-DXTOUCH_P4_C6_PREINITIALIZED_BT=ON`，其他 C6 固件由主控初始化控制器。

可使用 `python boards/build.py s3 --ble-provisioning` 构建；P4 使用 `python boards/build.py p4 --ble-provisioning`，预初始化控制器的 C6 再加 `--c6-preinitialized-bt`。蓝牙构建使用独立目录和 sdkconfig，不改变普通构建。CYD 不支持蓝牙模块，保留双 OTA 分区。

例如 `python boards/build.py s3 --without audio camera print_preview --icons original`。CYD 的默认能力位于 `src/features/config.h`，音频驱动默认启用，摄像头和缩略图默认关闭；`XTOUCH_ICON_SET=mdi` 环境变量可选择 MDI 图标。

后台只显示编译进入固件且硬件支持的选项。显示尺寸和驱动配置见[主板说明](../boards/README.md)。

### 内存与存储

P4 的 32 MB Flash 配置启用 ESP-IDF 的 32 位地址缓存映射，以支持两个 12 MiB OTA 槽。引导程序与应用应使用同一套构建配置；更新引导程序需使用 USB 完整刷写。

CYD 无 PSRAM；S3 有 8 MB，P4 有 32 MB PSRAM。S3/P4 的图像和解码数据使用 PSRAM，显示 DMA 和任务栈保留内部内存。摄像头全屏播放期间暂停缩略图工作及部分后台巡检，MQTT 与退出触摸保持运行。

MQTT 建连使用同步网络调用，连接期间可能短暂影响界面响应。CYD 读取配置时会释放打印机 TLS 以回收内存；S3/P4 默认保持连接。

<a id="release"></a>

## 打包与 OTA

| 主板 | 应用镜像 | USB 完整镜像 | 应用地址 |
|---|---|---|---|
| ESP32-2432S028R | `xtouch-2.8.bin` | `xtouch-2.8-full.bin` | `0x10000` |
| ESP32-S3-Touch-LCD-2.8 V2 | `esp32-s3-touch-lcd-2.8-v2.bin` | `esp32-s3-touch-lcd-2.8-v2-full.bin` | `0x20000` |
| ESP32-P4-WIFI6-Touch-LCD-4.3 | `esp32-p4-wifi6-touch-lcd-4.3.bin` | `esp32-p4-wifi6-touch-lcd-4.3-full.bin` | `0x20000` |

完整镜像从 `0x0` 写入，其中 P4 引导程序位于 `0x2000`，S3 位于 `0x0`。应用镜像用于后台、OTA 和 SD 更新。

| 主板目录 | Flash | OTA 应用槽 | 数据分区 |
|---|---|---|---|
| `esp32-2432s028r` | 4 MB | 2 × 1.875 MiB | 128 KiB |
| `esp32-s3-touch-lcd-2.8-v2` | 16 MB | 2 × 6 MiB | 3.875 MiB |
| `esp32-p4-wifi6-touch-lcd-4.3` | 32 MB | 2 × 12 MiB | 7.875 MiB |

各主板的自动更新清单位于 `boards/<主板目录>/ota.json`。S3/P4 的包元数据位于同目录 `manifest.json`。清单记录版本、大小及摘要，需与 Release 镜像一致。打包脚本检查芯片、应用大小、合并地址和分区边界。

Release 下载提供上述六个 BIN 文件；包元数据供构建和校验使用。安装、备份与恢复见[安装指南](INSTALLATION.md#install)。

<a id="website"></a>

## 刷机页

**Configuration and web installer** Actions 使用已有 Release 镜像构建并部署 GitHub Pages，不编译固件。

本地构建需要对应版本的 CYD 镜像，以及可选的 S3/P4 打包目录：

```sh
npm ci --prefix web
npm run bundle --prefix web
python scripts/build_web.py --firmware-dir release-images --board-firmware-dir boards/build-s3/artifacts --board-firmware-dir boards/build-p4/artifacts --output _site
python -m http.server 8895 --directory _site
```

打开 `http://localhost:8895/`。构建脚本校验镜像及清单，生成保留用户数据区的安装分段。刷写工具从源模块打包，依赖与 P4 下载器许可见 `web/flasher/NOTICE.md`。
