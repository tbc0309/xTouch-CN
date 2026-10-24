# ESP32-P4-WIFI6-Touch-LCD-4.3

| 项目 | 接线 / 配置 |
|---|---|
| LCD | ST7701 MIPI-DSI，2 lane / 500 Mbps，480×800；像素时钟 30 MHz |
| LCD 复位 / 背光 | GPIO27 / GPIO26，背光 5 kHz / 10 bit PWM |
| 触摸 | GT911：SDA 7、SCL 8，轮询 0x5D / 0x14，无独立 INT/RST |
| Wi-Fi | C6 / ESP-Hosted SDIO slot 1：CLK18、CMD19、D0..3=14..17、复位54 |
| SDMMC 卡 | slot 0 的 4 线：CLK43、CMD44、D0..3=39..42，与 C6 总线分开 |
| SD 电源 | Arduino P4 变体使用 GPIO45 低有效电源开关及 LDO4，与官方原理图一致 |
| 存储 | 32 MB Flash：双 12 MiB OTA、7.875 MiB SPIFFS；32 MB PSRAM，200 MHz |
| 按键 | BOOT（GPIO35）在程序运行后短按切换联网休眠／唤醒；POWER 为硬件电源开关，RESET 为硬件复位 |
| 音频 | ES8311：MCLK13、BCLK12、LRCK10、DOUT9，功放使能53；I²C 与触摸共用 |

构建：激活 ESP-IDF 6.1 后，在仓库根目录运行 `python boards/build.py p4`。

默认构建面向 **芯片版本 v3.x（最低 v3.0）**。ESP-IDF 6.1 将 1.x 和 3.x 配置分开；此项目发布的 3.x 镜像不能用于 1.x。[乐鑫芯片版本说明](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/esp32p4/api-reference/system/chip_revision.html)。更换板载 C6 固件时，应确认其 ESP-Hosted 版本与主控兼容。

UI 使用原生 **800×480 全屏画布**，复用共享设计坐标与布局，默认采用大屏 MDI 资源；文字、数字和主要图标以对应像素尺寸绘制。机型图片由统一源资源生成适合屏幕的版本。摄像头采用打印机网络图像流，不使用板载 CSI 接口；[支持范围](../README.md#摄像头)。CSI 和电池电量显示未加入此应用。

音频时钟使用晶振，触摸音和警告音默认关闭。应用代码和只读资源保留在 Flash，PSRAM 用于界面与图像缓冲。联网待机保留 MQTT；可选深睡支持触摸唤醒和复位键启动；低功耗核查询 GT911，主核保持深睡，背光保持关闭。启用自动深睡后的默认等待时间为 60 分钟。电源指示灯为硬件接线，不能软件关闭。
