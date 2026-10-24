# Waveshare 主板移植

使用 xTouch-CN 的相同 UI、MQTT、账号管理、网页后台和内部存储代码，通过独立硬件层接入两块主板。1.0.0 提供 S3 V2 与 P4 独立固件；下载前核对完整型号与芯片版本。

| 目标目录 | 硬件 | 显示策略 |
|---|---|---|
| `esp32-s3-touch-lcd-2.8-v2` | ESP32-S3，16 MB Flash、8 MB PSRAM，ST7789、CST3530，240×320 | 横屏 320×240 |
| `esp32-p4-wifi6-touch-lcd-4.3` | ESP32-P4 + C6，32 MB Flash、32 MB PSRAM，ST7701 MIPI、GT911，480×800 | 横屏 800×480，全屏布局及独立大屏素材 |

S3 仅针对 **V2 / CST3530**，不是 V1 / CST328，也不是带 B/C 后缀的其他主板。P4 的 PSRAM 频率为 200 MHz，默认目标为 芯片版本 v3.x（最低 v3.0）。ESP-IDF 6.1 中 1.x 与 3.x 的硬件配置互斥，不能共用同一镜像。本项目发布的 P4 镜像仅面向 v3.x。

## 构建

安装并激活 [ESP-IDF v6.1](https://github.com/espressif/esp-idf/releases/tag/v6.1)。Arduino-ESP32 组件版本为 3.3.12，UI 使用 LVGL 8.3.9；P4 主机端使用 ESP-Hosted 3.0.10，更新 P4 不会改写 C6 固件。源依赖版本保存在 `dependencies.json`，首次构建需要联网。

在项目根目录执行：

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
python scripts/embed_portal.py
python boards/build.py s3
python boards/build.py p4
```

Windows 与 Linux 使用相同脚本。构建后会在对应 `build-*/artifacts/` 中生成应用镜像、完整镜像和 SHA-256 清单。两个目标分别保存构建目录和 SDK 配置。S3 使用 6 MiB × 2 OTA 分区、3.875 MiB SPIFFS；P4 使用 12 MiB × 2 OTA 分区、7.875 MiB SPIFFS，分别使用完整的 16/32 MiB Flash。

P4 使用 200 MHz PSRAM 保存界面与图像缓冲；应用代码和只读资源保留在 Flash，不启用 PSRAM XiP。

## 摄像头

点击首页左上角 Wi-Fi 至摄像头图标区域打开，再点击右上角关闭。实现拓竹局域网 TLS 6000 JPEG 流，适用于支持该协议的 P1/A1 系列；需要当前设备配置中有打印机的**局域网 IPv4 和 8 位访问码**。若当前为 Cloud 设备，会查找相同序列号的已保存 LAN 设备以取得本地摄像头地址和访问码。仅有 Cloud Token 不能替代本地摄像头访问码。

P4 与 S3 均支持摄像头查看。JPEG 使用 PSRAM 缓冲和缩放解码，接收与解码在独立任务执行。设备切换时自动关闭旧流，退出后释放帧缓存。

已接入 X1/H2/P2 等机型的 RTSPS/H.264 原生播放，需 Baseline 码流与足够 PSRAM，详见 [范围与限制](CAMERA.md)。JPEG 摄像头兼容 P1S；RTSPS 的实机编码兼容性未确认；拓竹云视频和音频未实现。P4 的 H.264 **编码**硬件不能等同于 H.264 解码能力。摄像头采用打印机自签名 TLS，限制为私有 IPv4，不用于公网地址。

## 存储与升级

首次启动通过扫码配网及网页管理完成配置；电容触摸使用出厂坐标，无需校准。SD 卡用于配置同步、备份与卡刷。S3 SDMMC 为 1 线，P4 为 4 线；挂载失败不会格式化卡片。两板不支持 CYD 的外接 DS18B20 接线；可显示打印机报告的有效仓温。

两板使用独立的 OTA 清单和文件名，不能使用 CYD 的 `xtouch-2.8.bin`。文件名与刷写地址见[打包说明](../docs/DEVELOPMENT.md#release)。

## 待机与声音

默认联网待机保留打印唤醒。空闲深睡为可选功能，默认关闭；深睡期间断网，不能即时接收打印唤醒。启用后默认在联网待机 60 分钟后进入深睡。三板均支持触摸唤醒和复位键启动，不配置定时唤醒。P4 使用低功耗核查询触摸，主核不因查询周期而唤醒。

屏幕设置和后台可分别开启触摸音和警告音，两项默认关闭。S3/P4 的电源、充电指示灯由硬件控制，软件不提供开关。共享逻辑和可选模块见 [代码结构](../docs/DEVELOPMENT.md#architecture) 与 [编译选项](../docs/DEVELOPMENT.md#options)。

[主板功能对比与界面截图](../README.md#主板功能对比)

## 官方资料

- [S3 V2 官方资料](https://docs.waveshare.net/ESP32-S3-Touch-LCD-2.8/)
- [P4 官方资料](https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-4.3/)
- [P4 示例代码](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3)

P4 初始化表沿用微雪 Apache-2.0 示例，来源提交记录见各文件注释。代码仍遵循主项目许可证；JPEGDEC 的 Apache-2.0 许可证由上游依赖保留。
