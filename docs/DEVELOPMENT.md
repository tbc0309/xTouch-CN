# 开发、构建与打包

简体中文 | [English](DEVELOPMENT.en.md) | [文档目录](README.md)

CYD、S3、P4 及ES3C35P 使用原生 ESP-IDF 6.1、LVGL 9.6.0。P4 使用 Hosted 3 主机端。

<a id="build"></a>

应用与LVGL组件共用同一组主板能力定义。原生构建检查会在打包前拒绝不一致的配置，避免界面资源和内存预算出现偏差。
## 构建

CYD 按实际面板分别构建：`python boards/build.py cyd` 为 ILI9341（保留 ILI9341_2 参数），`python boards/build.py cyd-st7789` 为 ST7789。两者共用原生硬件驱动，项目身份、固件文件名和 OTA 目录独立，防止自动升级互换面板固件；不要仅凭 USB 插口数量判断型号。ST7789 不改变 40 MHz SPI、16 行 DMA 或触摸校准。

激活 ESP-IDF 6.1 工具环境，在仓库根目录执行：

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
python boards/build.py cyd
python boards/build.py s3
python boards/build.py p4
python boards/build.py es3c35p
```

构建入口自动从可读源码生成对应语言的后台资源，随后准备依赖、配置模块、审计和打包，产物位于 `boards/build-<板型>/artifacts/`。首次构建依赖联网；版本锁定见 `boards/dependencies.json`、锁文件和组件清单。普通构建使用仓库生成资源，不需要源 TTF。

[LCDWIKI ES3C35P](../boards/lcdwiki-es3c35p/README.md) 使用 `python boards/build.py es3c35p`。它与 Waveshare S3 的项目身份和镜像独立，不能因芯片相同而互刷。

<a id="architecture"></a>
## 代码结构

主循环和 LVGL 在 CPU1 运行，摄像头解码任务固定在 CPU0；工作任务通过数据交换通知界面，不直接操作 LVGL。摄像头使用三个独立画布，在互斥锁内交换已完成的缓冲区；解码器每帧重新取得可写画布，避免复制整帧或覆盖正在显示的内容。正式构建审计核对生成配置中的任务核心。

| 目录 | 职责 |
|---|---|
| `src/ui/` | 共用页面、比例、语义配色、字库与交互 |
| `src/features/` | 可选功能、协议策略与诊断 |
| `src/platform/idf/` | 自有 IDF 网络、存储和运行工具 |
| `src/xtouch/` | 状态、配置、账号和路由集成 |
| `boards/common/` | 硬件接口与生命周期 |
| `boards/<主板>/` | 引脚、面板、触摸、音频、SDK 默认值 |
| `portal/`／`web/` | 设备管理网页／公开安装与查询网站 |
| `resources/`／`scripts/` | 生成输入、授权、生成及审计脚本 |

主板适配共用功能与界面组件。资源档位独立于物理分辨率，`ui/chrome_metrics.h` 提供共享尺寸；DMA 提交后的缓冲在传输完成前由驱动持有。

<a id="options"></a>
## 功能与资源选项

```sh
python boards/build.py s3 --without audio --assets small --icons original
```

| 模块 | 构建入口默认 |
|---|---|
| `cloud_token`／`cloud_login` | ON，独立编译 Token 与账号／验证码登录 |
| `xiaomi_ble` | ON，温湿度计接收 |
| 外接腔温 `--external-chamber` | OFF；独立 CYD GPIO22 DS18B20 模块 |
| `audio`／`deep_sleep`／`indicator` | ON；运行开关及硬件能力另行决定 |
| `camera`／`print_preview` | CYD OFF，S3／P4／ES3C35P ON |
| 蓝牙配网 `--ble-provisioning` | OFF；仅S3／ES3C35P，与温湿度计构建互斥 |

外接有线模块使用 `settings.chamberTemp` 启用、`settings.chamberTempDiff` 校准；未编译时这些参数不启用探头。屏幕温湿度计下拉框使用独立的 `environmentSensor.display`，不切换有线模块。

`--without` 从编译中移除模块，不只是隐藏界面。默认 CYD／S3 为 small original，P4／ES3C35P 为 large MDI；可用 `--assets small|large`、`--icons original|mdi` 分别选择。CYD 固定中英文、打印单 Logo、列表选择；S3／P4／ES3C35P 保留多语和图片资源。

资源组合仍须通过目标分区、字库及运行内存审计；尤其 CYD 不保证任意组合都能容纳，不能绕过容量检查。


### 字库与生成资源

文件名字库通过 CMake 选项 `XTOUCH_FILENAME_FONT=auto|common-zh|ui` 选择。标点补充由 `XTOUCH_FILENAME_PUNCTUATION` 独立控制，默认 `ON`，覆盖 99 个常用中英文标点及符号；选择 `ui` 关闭常用汉字扩展时，仍保留标点和完整的界面固定文案。`auto` 按主控判断：ESP32-S3、ESP32-P4 的文件浏览模块默认内置 GB2312 的 6763 个常用简体汉字及 GB2312 符号、常用排版标点，其它主控默认仅含界面用字；不依据某个板名或资源大小判断。未编译文件浏览模块时不增加该字库容量，也不会由字库选项开启模块。补充模块只保存当前同字号 UI 未覆盖的字形；已有 UI 字形通过字体链复用，避免重复占用固件。共享资源位于 `resources/fonts/file-names/`，使用 `scripts/generate_filename_fonts.py` 重新生成。更新 UI 字库的生成流程会自动增减补充模块；构建时校验 `coverage.json`，覆盖不一致时拒绝使用旧资源。28px 字库通过静态 fallback 链分段，兼容 small／large 资源的字形索引范围，不改变全局 LVGL 字形结构。

界面文字为 OPPO Sans 4.0、字重 600；日／韩缺字采用 Noto Sans CJK。CYD 仅保留中英文字库。字体及图标按资源档位生成，机型图也预生成，运行时不再依靠任意缩放补救错误资源。

翻译元数据位于 `resources/languages.json`；持久化语言索引不可随意重排。修改后运行生成器及 `scripts/check_localization.py`。字体生成需要合法源字体，命令见生成脚本 `--help`；授权见[第三方许可](../LICENSE-3RD-PARTY.md)。

## 构建检查

```sh
python scripts/check_localization.py
python scripts/check_documentation.py
python boards/check_contracts.py
python scripts/check_native_idf.py boards/build-s3 --release
```

原生审计检查框架依赖、模块选择、资源、镜像身份和分区容量。构建目录按所选主板替换。

## 诊断工具

| 范围 | 工具 |
|---|---|
| Flash 与 RAM 占用 | `idf.py size`、`idf.py size-components`、`idf.py size-files` |
| 静态分析 | `compile_commands.json` 与 [IDF Clang-Tidy](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-clang-tidy.html)，使用匹配的 esp-clang 工具链 |
| 堆与任务栈 | IDF heap tracing、完整性检查及任务栈余量 |
| LVGL 性能 | LVGL Kconfig 中的 SYSMON／PERF_MONITOR 或 profiler |

诊断选项会影响内存占用和绘图性能；普通固件不包含测试端点。

<a id="release"></a>
## 分区与打包

| 板型 | Flash | OTA 应用槽 | 数据分区 |
|---|---|---|---|
| CYD | 4 MB | 2 × 1.875 MiB | 128 KiB |
| S3 | 16 MB | 2 × 6 MiB | 3.875 MiB |
| P4 | 32 MB | 2 × 12 MiB | 7.875 MiB |
| ES3C35P | 16 MB | 2 × 6 MiB | 3.875 MiB |

应用地址 CYD `0x10000`，S3／P4／ES3C35P `0x20000`；完整包从 `0x0` 安装。P4 引导程序在 `0x2000`。使用打包输出，不手工拼接不匹配的引导与应用。P4 的缓存映射与 64 字节 L2 行配置由构建审计。

打包器为每块主板生成对应的版本、长度、校验值及 `ota.json` 清单。固件文件名见[安装表](INSTALLATION.md#install)。

<a id="website"></a>
## 网页与文档

```sh
npm run bundle --prefix web
python boards/sync_release_metadata.py --check
python scripts/build_web.py --firmware-dir boards/build-cyd/artifacts --ota-manifest boards/build-cyd/artifacts/ota.json --board-firmware-dir boards/build-s3/artifacts --board-firmware-dir boards/build-p4/artifacts --board-firmware-dir boards/build-es3c35p/artifacts --output _site
python -m http.server 8895 --directory _site
```

网站使用已有固件及匹配清单生成。界面示例见[图集](SCREENSHOTS.md)，故障码页面由源数据库生成。

### 连接诊断

`mqttTiming.connected` 是实时 MQTT 传输状态；`attemptConnected` 只表示上一次连接尝试结果。`statusReceived` 区分尚未收到状态，`statusExpired` 与 `reportAgeMs` 描述报告新鲜度；不要把上次尝试成功或旧状态时间当作当前连接正常的证据。独立 JSON 库 ArduinoJson 不依赖 Arduino 框架。

## 共用UI组件

界面组件通过以下入口共用尺寸、配色和字库：

| 内容 | 入口 |
|---|---|
| 间距、内边距、圆角与控件尺寸 | `src/ui/ui_specs.h`：`ui_specs_get()` |
| 标题、关闭按钮与工具栏 | `src/ui/chrome_metrics.h` |
| 语义配色与状态 | `src/ui/palette.h` |
| 字库与字号层级 | `src/ui/native_fonts.h`、`src/ui/text_fit.h` |
| 图标系列与资源档位 | `src/ui/symbol_fonts.h`、`resources/ui-assets/` |
| 图标＋文字按钮 | `ui_layout_action_fit()`；图标固定，超长时只滚动文字 |
| 动态页面普通按钮标题 | `ui_layout_setup_caption()`；尺寸变化时仍由共用 C 布局居中 |
| 配网选择卡与二维码 | `ui_layout_setup_choice_content()` / `ui_layout_qr_size()`；原生尺寸、固定图标与独立滚动文字 |
| 耗材卡片内容 | `ui_specs_material_content()`；色条、均衡的读数／材料行与底边距 |

以上接口使用原生像素；设计坐标只在边界转换一次。资源缩放用于选择画布的字号层级，长动作文字使用当前字号滚动显示。动态修改标题后调用同一布局接口。普通单标题按钮由组件创建钩子统一规范化，动态页面工厂也调用同一份共享C实现。数字遥测保留独立的紧凑数据布局。

[主板适配接口](PORTING.md)

### 文件浏览与像素屏模块

ESP32-S3 / ESP32-P4 标准构建默认启用文件浏览和像素屏联动；CYD 因存储容量限制默认关闭。`--without gcode_browser pixel_screen` 可关闭，`--gcode-browser --pixel-screen` 可显式启用。文件浏览需要可用SD卡；像素屏与蓝牙配网互斥，温湿度计与像素屏共用蓝牙生命周期和主机租约。蓝牙配网构建默认关闭像素屏。直接 CMake 构建使用 `XTOUCH_FEATURE_GCODE_BROWSER` 与 `XTOUCH_FEATURE_PIXEL_SCREEN`；`boards/common/interactive_modules.cmake` 在选择 SDK 蓝牙默认值前决定模块。

文件浏览仅在主板已有 `storage.cpp` 存储适配时默认启用。未映射SD的目标不编入文件浏览及其文件名字库消费者；像素屏模块独立选择。
