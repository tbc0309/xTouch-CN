# LCDWIKI ES3C35P · ESP32-S3 3.5 寸

简体中文 | [English](README.en.md)

使用原生 ESP-IDF 6.1、LVGL 9.6.0，与 S3 相同的可选功能模块和共用界面，默认大屏 MDI 资源。支持电容触摸、扬声器、RGB灯、联网待机触摸唤醒和配置保留 OTA。

## 硬件

| 项目 | 配置 |
|---|---|
| 芯片与存储 | ESP32-S3、16 MB Flash、8 MB Octal PSRAM |
| 显示 | ST77922，原生320×480，软件旋转为480×320，QSPI40 MHz |
| QSPI | CS10、CLK12、D0=11、D1=13、D2=14、D3=9 |
| 背光 | GPIO41，20 kHz PWM |
| 触摸 | ST77922触摸接口0x55，I²C SDA38/SCL39，RST48/INT47；完整读取控制器点数据 |
| 扬声器 | ES8311地址0x18；MCLK17、BCLK18、WS21、DOUT15；功放使能GPIO1低有效 |
| RGB灯 | 一颗WS2812，GPIO40，使用RMT，不占LCD的SPI总线 |
| SD卡 | 4-bit SDMMC：CLK5、CMD4、D0=6、D1=7、D2=2、D3=3 |
| 控制键 | BOOT GPIO0、RESET；USB Serial/JTAG控制台 |

联网待机可用触摸唤醒。INT47不是RTC引脚，深睡使用BOOT键唤醒，不宣称深睡触摸唤醒；深睡为可选模块，默认不自动进入。麦克风和电池测量未纳入本次适配。

## 构建与镜像

```sh
python boards/build.py es3c35p
```

也接受 `esp3c35p` 别名。应用为 `lcdwiki-es3c35p.bin`，USB完整包为 `lcdwiki-es3c35p-full.bin`，位于 `boards/build-es3c35p/artifacts/`。配置保持16 MB双6 MiB OTA槽。OTA 更新使用应用固件；完整包用于 USB 安装。

同为ESP32-S3不表示可与Waveshare S3互刷。此目标的应用项目标识为 `xtouch_es3c35p`；打包和应用更新同时核对芯片、项目及目标文件名。

## 显示与传输

[厂商资料](https://www.lcdwiki.com/3.5inch_ESP32-S3_Display) · [硬件初始化许可](LICENSE.hardware-init)

该玻璃不依赖MADCTL轴交换；旋转和RGB565字节序在PSRAM合成阶段处理。绘图块与在途DMA缓冲分离，所有标题、卡片、弹窗及设备选择仍使用共用UI。触摸坐标随显示方向映射；提示音与状态指示灯分别控制。

QSPI 不直接从 PSRAM 取 DMA 数据：请求5 KiB的传输上限，由IDF按DMA描述符向上取整至不超过8 KiB，并以最多两个内部 DMA 块流水发送，避免外部内存带宽不足产生 TX underflow。PSRAM 合成缓冲仍与在途数据分离。
