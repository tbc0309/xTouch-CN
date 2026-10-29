# ESP32-P4-WIFI6-Touch-LCD-4.3

简体中文 | [English](README.en.md)

| 项目 | 接线 / 配置 |
|---|---|
| LCD | ST7701 MIPI-DSI，2 lane / 500 Mbps，480×800；像素时钟 30 MHz |
| LCD 复位 / 背光 | GPIO27 / GPIO26，背光 5 kHz / 10 bit PWM |
| 触摸 | GT911：SDA 7、SCL 8，轮询 0x5D / 0x14，无独立 INT/RST |
| Wi-Fi | C6 / ESP-Hosted SDIO slot 1：CLK18、CMD19、D0..3=14..17、复位54 |
| SDMMC 卡 | slot 0 的 4 线：CLK43、CMD44、D0..3=39..42，与 C6 总线分开 |
| SD 电源 | GPIO 驱动使用 GPIO45 低有效电源开关及 LDO4，与官方原理图一致 |
| 存储 | 32 MB Flash：双 12 MiB OTA、7.875 MiB SPIFFS；32 MB PSRAM，200 MHz |
| 按键 | BOOT（GPIO35）在程序运行后短按切换联网休眠／唤醒；POWER 为硬件电源开关，RESET 为硬件复位 |
| 音频 | ES8311：MCLK13、BCLK12、LRCK10、DOUT9，功放使能53；I²C 与触摸共用 |

## 目标与构建

本目标要求 P4 v3.x，最低 v3.0。横屏 800×480，默认大屏 MDI 资源。当前采用 Hosted 3 主机端。MIPI、C6 SDIO、SD 卡和低功耗触摸各自管理生命周期，不复用错误的总线或电源引脚。

激活 ESP-IDF 6.1，在仓库根目录运行 `python boards/build.py p4`。当前使用 LVGL 9.6.0，无 Arduino 框架。功能模块和资源档位可选择，见[开发指南](../../docs/DEVELOPMENT.md#options)。

电容触摸不使用 CYD 校准。声音默认关闭；普通待机保持网络，深睡模块可独立编译，自动深睡默认不触发。硬件电源／充电指示灯不能作为软件 RGB 指示灯控制。

[安装与镜像](../../docs/INSTALLATION.md#install) · [摄像头范围](../CAMERA.md) · [共用界面](../../docs/DEVELOPMENT.md) · [硬件说明](../../docs/HARDWARE.md)
