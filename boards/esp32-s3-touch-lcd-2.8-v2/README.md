# ESP32-S3-Touch-LCD-2.8 V2

简体中文 | [English](README.en.md)

| 项目 | 接线 / 配置 |
|---|---|
| LCD | ST7789，MOSI 45、SCLK 40、CS 42、DC 41、RST 39，80 MHz SPI |
| 背光 | GPIO5，20 kHz / 10 bit PWM |
| 触摸 | **CST3530 / V2**，原生 I²C：SDA 1、SCL 3、RST 2、INT 4，地址 0x58 |
| SDMMC | 1 线：CLK 14、CMD 17、D0 16；D3 21 拉高 |
| 电池供电保持 | GPIO7 拉高，休眠时保持电池供电 |
| 按键 | GPIO6 电源键在程序运行后短按切换联网休眠／唤醒；RESET 为硬件复位 |
| 存储 | 16 MB Flash、8 MB 八线 PSRAM；256 KB LVGL 内存池放入 PSRAM |
| 音频 | PCM5101：BCLK48、LRCK38、DOUT47，无软件功放使能 |

## 目标与构建

本目标只适配 V2／CST3530。横屏 320×240，默认小屏 original 资源。LVGL 使用内部部分绘图缓冲，在 PSRAM 合成后按面板方向 DMA 提交；触摸映射独立于显示资源。GPIO7 保持电池电源，GPIO6 短按联网待机。

激活 ESP-IDF 6.1，在仓库根目录运行 `python boards/build.py s3`。当前使用 LVGL 9.6.0，无 Arduino 框架。功能模块和资源档位可选择，见[开发指南](../../docs/DEVELOPMENT.md#options)。

电容触摸不使用 CYD 校准。声音默认关闭；普通待机保持网络，深睡模块可独立编译，自动深睡默认不触发。硬件电源／充电指示灯不能作为软件 RGB 指示灯控制。

[安装与镜像](../../docs/INSTALLATION.md#install) · [摄像头范围](../CAMERA.md) · [共用界面](../../docs/DEVELOPMENT.md) · [硬件说明](../../docs/HARDWARE.md)
