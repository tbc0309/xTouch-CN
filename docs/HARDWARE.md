# 主板与外设

简体中文 | [English](HARDWARE.en.md) | [文档目录](README.md)

核对完整型号、显示芯片、触摸芯片与 GPIO。尺寸相同或外观相似不代表镜像兼容，固件不会自动识别任意屏幕。

<a id="cyd"></a>
## CYD · ESP32-2432S028R

经典 ESP32、4 MB Flash、无 PSRAM，ILI9341 显示与 XPT2046 电阻触摸，横屏 320×240。面板由 ESP-IDF SPI 驱动，采用与 ILI9341_2 对应的初始化配置；显示与触摸均使用原生驱动。首次启动需要校准。

| 外设 | 信号 | GPIO |
|---|---|---|
| 屏幕 | MOSI／SCLK／CS／DC／RST | 13／14／15／2／12 |
| 背光 | PWM | 21 |
| 触摸 | MOSI／MISO／CLK／CS／IRQ | 32／39／25／33／36 |
| SD | MOSI／MISO／SCLK／CS | 23／19／18／5 |

屏幕 SPI 为 40 MHz。触摸使用独立引脚采样；显示、触摸和背光实现位于[板级驱动](../boards/esp32-2432s028r/display.cpp)。保留双 OTA，界面仅中英文、列表设备选择及单张打印 Logo。摄像头、任务缩略图和完整故障说明未编入。该板未配置可靠电量计，不推测显示电池百分比。

<a id="waveshare"></a>
## Waveshare S3 V2 与 P4

| 主板 | 已适配硬件 |
|---|---|
| ESP32-S3-Touch-LCD-2.8 V2 | ESP32-S3，16 MB Flash／8 MB PSRAM，ST7789／CST3530，横屏 320×240 |
| ESP32-P4-WIFI6-Touch-LCD-4.3 | P4 v3.x（最低 v3.0）＋C6，32 MB Flash／32 MB PSRAM，ST7701 MIPI／GT911，横屏 800×480 |

S3 V1/CST328、B/C 变体及其他 P4 芯片修订不自动兼容。两板无需 CYD 校准，不能沿用 CYD 传感器引脚。P4 使用 Hosted 3 主机端。

| 板级资料 | 地址 |
|---|---|
| S3 | [驱动与配置](../boards/esp32-s3-touch-lcd-2.8-v2/README.md) |
| P4 | [驱动与配置](../boards/esp32-p4-wifi6-touch-lcd-4.3/README.md) |
| 共用接口 | [board.h](../boards/common/board.h) |

### 外壳

[S3 外壳](../cases/case_esp32-s3-touch-lcd-2.8-v2.zip)提供不同壁厚、USB／TF 开口及扬声器版本；[P4 外壳](../cases/case_esp32-p4-wifi6-touch-lcd-4.3.zip)提供 40-pin 开口及扬声器变体。按主板与接口选择，不适用于任意同尺寸板。

<a id="sensor"></a>
## 外接仓温

CYD 的 DS18B20 支持是独立的[外接腔温模块](../src/features/external_chamber/sensor.h)，使用 GPIO22 的原生 OneWire，默认不编译。定制构建可用 `python boards/build.py cyd --external-chamber` 选择该模块；它不等同于 BLE 温湿度计，也不增加打印机仓室加热能力。标准固件设置中的“温湿度计”控制后台已绑定的 BLE 传感器显示模式。打印机自身上报的腔温不受此可选模块开关影响。

![DS18B20 引脚示意](../readme-assets/DS18B20_pinout.png)

## 新板适配

分辨率、资源大小、图标风格和模块能力独立配置。新板须核对原厂原理图、面板初始化、触摸坐标、音频、供电与启动引脚，再完成编译、模拟和实机验证；未完成前不列为发行支持板型。见[开发规范](DEVELOPMENT.md)。

## LCDWIKI ES3C35P

ESP32-S3、ST77922 3.5 寸、16 MB Flash／8 MB PSRAM，480×320，默认大屏资源，与现有 S3 使用相同功能模块。硬件与构建见[板级说明](../boards/lcdwiki-es3c35p/README.md)。此板与 Waveshare S3 引脚及固件标识不同，请使用对应主板固件。

## 保存数量上限

CYD 最多保存20台设备、10个账号；S3、P4 和 ES3C35P 最多100台设备、50个账号。实际容量还受可用内存与凭据长度限制，一次连接一台打印机。
