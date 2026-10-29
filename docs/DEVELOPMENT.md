# 开发、构建与打包

简体中文 | [English](DEVELOPMENT.en.md) | [文档目录](README.md)

CYD、S3、P4 及ES3C35P 使用原生 ESP-IDF 6.1、LVGL 9.6.0。P4 使用 Hosted 3 主机端。

<a id="build"></a>

应用与LVGL组件共用同一组主板能力定义。原生构建检查会在打包前拒绝不一致的配置，避免界面资源和内存预算出现偏差。
## 构建

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

共享功能不复制到每块板。像素尺寸与资源档位分开；标题／控件采用 `ui/chrome_metrics.h`，仅在边界转换坐标一次。驱动必须在 DMA 完成前保持缓冲有效，不为无 PSRAM 的 CYD 盲目增加整帧缓冲。

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

界面文字为 OPPO Sans 4.0、字重 600；日／韩缺字采用 Noto Sans CJK。CYD 仅保留中英文字库。字体及图标按资源档位生成，机型图也预生成，运行时不再依靠任意缩放补救错误资源。

翻译元数据位于 `resources/languages.json`；持久化语言索引不可随意重排。修改后运行生成器及 `scripts/check_localization.py`。字体生成需要合法源字体，命令见生成脚本 `--help`；授权见[第三方许可](../LICENSE-3RD-PARTY.md)。

## 验证

1. 运行受影响协议、认证、隔离与 UI 回归；共用改动覆盖 CYD、S3、P4 及受影响的其他主板。
2. 编译普通目标，运行 `scripts/check_native_idf.py <构建目录> --release`，审计无 Arduino、无诊断入口、正确资源、分区及模块。检查包括 IDF 响应文件中的测试参数，以及最终固件中的网页字节是否与源码和模块组合一致。
3. 从当前生产 UI 渲染截图，检查文字墨迹、对齐、截断、滚动与内存。
4. 在目标硬件验证启动、配置保持、网页、网络、显示与受影响功能；不能用模拟通过代替实机。
5. 固件更新前执行 `python scripts/check_thumbnail_upstream.py --latest-release --strict --report boards/build/thumbnail-upstream-review.json --cache boards/build/upstream-sources`。该工具需要已认证的 GitHub CLI，或只读 GitHub API 凭据环境变量 `GH_TOKEN`／`GITHUB_TOKEN`。上游相关代码变化须先复核，不能只刷新基线；与模拟器协议联动的改动同步可复用回归。

真实打印机测试一次只启用一个客户端，备份配置，轮次之间断开并静置，结束恢复。控制命令与无害查询分开；不自动重试机械操作。未验证的机型、认证或网络故障保留未完成状态。


发布前检查固件编译、网页管理、配网、界面布局及 OTA 分区切换；硬件测试与私有诊断记录单独保存在本地。

## 分析工具与预算

| 目标 | 工具与使用边界 |
|---|---|
| Flash、IRAM、DRAM 和 OTA 容量 | 在对应主板工程中运行 `idf.py size`、`idf.py size-components`、`idf.py size-files`；同时核对最终应用镜像和分区，不以源码行数推断容量 |
| 静态诊断 | 保留 GCC 警告及 `compile_commands.json`；[IDF Clang-Tidy](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-clang-tidy.html) 需要匹配的 esp-clang 工具链，在独立构建目录分析，不直接切换发行版工具链或自动修复第三方代码 |
| 堆、碎片和栈 | 同时记录空闲量、历史最低值、最大连续块和任务栈余量；实验构建可用 IDF heap tracing／完整性检查，TLS 成功所需的是连续可用内存 |
| LVGL 绘制 | 按当前 LVGL 的 Kconfig 启用 SYSMON／PERF_MONITOR 或 profiler，仅用于诊断；记录刷新区域、绘图时间与驱动传输时间，不把模拟器耗时当实体屏帧率 |
| 布局和字库 | 用原生 LVGL 运行生产组件，覆盖长名称、100 台设备、各语言、极值读数、滚动与命中；直接检查实际字体的 glyph，不能只确认字符串包含所需字符 |
| DMA 和异步任务 | 核对缓冲所有权、完成回调、失败回退、取消与退出；压力测试同时观察网络和内存，避免只测总线空载吞吐 |

分析开关、追踪缓冲和测试端点不进入普通固件。每次优化比较同一资源、同一配置和同一工作负载；不能把更改字体、关闭功能或扩大内存后的结果当作原条件优化。

<a id="release"></a>
## 分区与打包

| 板型 | Flash | OTA 应用槽 | 数据分区 |
|---|---|---|---|
| CYD | 4 MB | 2 × 1.875 MiB | 128 KiB |
| S3 | 16 MB | 2 × 6 MiB | 3.875 MiB |
| P4 | 32 MB | 2 × 12 MiB | 7.875 MiB |
| ES3C35P | 16 MB | 2 × 6 MiB | 3.875 MiB |

应用地址 CYD `0x10000`，S3／P4／ES3C35P `0x20000`；完整包从 `0x0` 安装。P4 引导程序在 `0x2000`。使用打包输出，不手工拼接不匹配的引导与应用。P4 的缓存映射与 64 字节 L2 行配置由构建审计。

每板 `ota.json`、镜像摘要、长度、版本和 Release 产物必须一致。文件名见[安装表](INSTALLATION.md#install)。只有通过发布检查的构建才能更新公开下载及 OTA 清单。编译、模拟、实机证据分别记录。

<a id="website"></a>
## 网页与文档

```sh
npm run bundle --prefix web
python boards/sync_release_metadata.py --check
python scripts/build_web.py --firmware-dir boards/build-cyd/artifacts --ota-manifest boards/build-cyd/artifacts/ota.json --board-firmware-dir boards/build-s3/artifacts --board-firmware-dir boards/build-p4/artifacts --board-firmware-dir boards/build-es3c35p/artifacts --output _site
python -m http.server 8895 --directory _site
```

公开网站打包读取已有镜像，不替代固件构建。检查选择板型、保留数据分段、配置生成器的能力及后台 Token 与 BLE Key 教程双语链接。README 展示六张独立 P4 截图，每行两张，完整示例在[图集](SCREENSHOTS.md)。故障码目录由源数据生成，内部验收记录不放进用户指南。

### 连接诊断

`mqttTiming.connected` 是实时 MQTT 传输状态；`attemptConnected` 只表示上一次连接尝试结果。`statusReceived` 区分尚未收到状态，`statusExpired` 与 `reportAgeMs` 描述报告新鲜度；不要把上次尝试成功或旧状态时间当作当前连接正常的证据。独立 JSON 库 ArduinoJson 不依赖 Arduino 框架。

## 共用UI组件

新增界面从共用规格取得参数，不为每个控件重新选择一套数值：

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

以上接口使用原生像素；设计坐标只在边界转换一次。资源缩放用于选择画布的字号层级，动作文字过长不再缩小该字号。动态修改标题后调用同一布局接口。普通单标题按钮由组件创建钩子统一规范化，动态页面工厂也调用同一份共享C实现。数字遥测保留独立的紧凑数据布局。

## 回归检查

发布前检查固件编译、网页管理、配网、界面布局及 OTA 分区切换；硬件测试与私有诊断记录单独保存在本地。
