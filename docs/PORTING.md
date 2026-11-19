# 主板移植接口

主板适配文件放在 `boards/<目录>/`，公共协议、界面和存储生命周期放在 `src/`。新增主板不应复制公共模块，也不应使用另一款主板的宏来借用引脚或电源行为。登记构建身份不代表已经完成硬件适配或实物验证。

## 构建身份

统一身份由 [board_catalog.py](../boards/board_catalog.py) 提供。现有正式主板保留原目录、固件名和分区。扩展目标可登记到 `boards/port_targets.json` 或 `boards/bsp_targets.json`；不存在的目录文件不会增加主板。目标键、目录、文件名以及同主控下的工程名必须唯一。元数据至少包含 `directory`、`stem`、`chip`、`chip_id`、`project`、`flash`、`screen`，并沿用现有条目的名称、提示、下载偏移等打包信息。

`releaseChannel` 为 `main` 或 `test`；扩展目标默认属于测试通道。`releaseSupported`、`localOnly`、`nativeBuildAllowed` 使用布尔值。编译状态和实物验收状态须分别记录，不能用成功编译代替触摸、颜色、电源和唤醒验证。

```sh
python boards/build.py s3 --dry-run
python boards/build.py s3 --jobs 4
python boards/build.py s3 --without audio camera --jobs 4
python boards/build.py s3 --ble-provisioning
```

`--dry-run` 只显示目标、资源及模块选择，不写配置、不启动 SDK。`--jobs` 控制本次编译的并发数。温湿度计与蓝牙配网保持互斥；选蓝牙配网会关闭温湿度接收。资源可用 `--assets small` 或 `--assets large` 明确选择。

板级根工程应在 `project(...)` 前包含 `boards/common/idf_project.cmake`；该入口初始化IDF并传递模块选择到独立依赖扫描。不要直接绕过它包含SDK的project.cmake，否则关闭模块时的依赖可能与应用不一致。SDK默认配置仍由板级工程声明。

## 适配文件

| 文件 | 责任 |
| --- | --- |
| `CMakeLists.txt`、`sdkconfig.defaults` | 选择真实主控、SDK能力、Flash/PSRAM和模块默认值。 |
| `partitions.csv` | 保留双OTA及数据分区；上传上限从两个等长OTA槽的实际尺寸生成，不按Flash容量猜测。 |
| `display.cpp` | 实现 [board.h](../boards/common/board.h) 中的显示、触摸、背光与休眠接口。 |
| `storage.cpp`（可选） | 实现 [storage_profile.h](../boards/common/storage_profile.h)，只描述SPI/SDMMC接线与板级电源。缺省为无SD，不继承其它板引脚。 |
| `power.h`（可选） | 实现时钟策略、深睡前后处理及触摸唤醒；缺省无GPIO操作和触摸深睡唤醒。 |
| `network.h`（可选） | 实现 `board_network_begin()`；带协处理器的板在此初始化板级供电/链路。 |
| `audio.cpp`、`indicator.cpp`（可选） | 实现相应驱动；未具备的硬件应关闭对应模块。 |
| `board_sources.cmake`（可选） | 设置 `BOARD_DISPLAY_SOURCE`，向 `BOARD_EXTRA_SOURCES`、`BOARD_EXTRA_DEFINITIONS`、`BOARD_PRIVATE_DEFINITIONS`、`BOARD_REQUIRES`、`AUDIO_REQUIRES` 添加所需驱动/组件。仅在板级声明，不修改应用协议。 |

外部SDK/BSP也应遵守相同边界。特殊电源、网络头可通过 `XTOUCH_BOARD_POWER_HEADER`、`XTOUCH_BOARD_NETWORK_HEADER` 选择；头文件必须实现相同接口。功能默认值应在板级工程进入公共组件前声明，不在源码内散布主板名称判断。

存储挂载、失败退役、文件句柄租约和延后卸载由公共实现负责。适配层 `configure` 失败或挂载失败后会调用 `release`，后者须允许重复调用；不能在仍有打开文件时释放卡或电源。P4的SD卡和Wi-Fi SDIO共享控制器修正在公共构建文件中维护，不修改SDK源树。

## 可选模块与OTA

摄像头开启且主控支持时，应用才声明对应H.264依赖；其它支持摄像头的目标可保留JPEG并返回已有的不支持格式提示。蓝牙配网应用代码及依赖也由模块开关显式选择。依赖下载与编译/链接是不同阶段，可选SDK使用 `require: no` 固定版本，由公共CMake明确选择应用依赖；组件管理器仍可能构建下载的静态库，不代表该模块已启用或其实现进入最终ELF。

[ota_channels.json](../resources/ota_channels.json) 同时定义仓库和通道。正式版仅使用 `main`，测试版使用 `main`、`test`；版本后缀须与通道一致。Fork应修改仓库身份，生成器和打包器会采用相同地址。生成的板级头文件、固件名、HTTP上限和分区必须一致；未知主板直接报错，不回退到CYD。测试版本不得通过正式版清单进入正式固件。

## 验证顺序

1. 核对厂家引脚、控制器、存储容量、面板时序和触摸坐标，不以相似板代替资料。
2. 先执行 `python boards/check_contracts.py`，使用 `--dry-run` 检查资源和模块，再完整构建并运行 `scripts/check_native_idf.py <构建目录> --release`。
3. 核对ELF/应用身份、容量、真实资源、模块关闭、OTA仓库/通道和应用与LVGL配置一致性。
4. 有实物后验证显示、触摸、声音、状态灯、联网、SD卡失败/拔出、待机/唤醒和配置保留；分别标明未实测项。

构建不会更改用户的全局Git配置。Git依赖在子进程中禁用自动CRLF转换，以保持固定源码的字节摘要一致。
