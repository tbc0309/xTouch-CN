# 原生 ESP-IDF 主板

各主板共用页面、打印机协议、网页管理和配置逻辑，板级目录只提供硬件与资源能力。固件使用 ESP-IDF 6.1、LVGL 9.6.0；P4 使用 Hosted 3 主机端。

| 目录 | 硬件 | 横屏与默认资源 |
|---|---|---|
| `esp32-2432s028r` | ESP32／ILI9341／XPT2046，4 MB Flash，无 PSRAM | 320×240，small original，中英文 |
| `esp32-s3-touch-lcd-2.8-v2` | S3／ST7789／CST3530，16 MB Flash，8 MB PSRAM | 320×240，small original |
| `esp32-p4-wifi6-touch-lcd-4.3` | P4 v3.x＋C6／ST7701／GT911，32 MB Flash，32 MB PSRAM | 800×480，large MDI |
| `lcdwiki-es3c35p`（实验） | S3／ST77922，16 MB Flash，8 MB PSRAM；RGB／ES8311 | 480×320，large MDI |

资源档位与分辨率独立选择，不根据同尺寸商品外观猜测兼容。S3 V1、B/C 变体以及非 v3.x P4 不能直接使用这些镜像。

## 构建

激活 ESP-IDF 6.1，在仓库根目录运行：

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
python scripts/embed_portal.py
python boards/build.py cyd
python boards/build.py s3
python boards/build.py p4
python boards/build.py es3c35p
```

构建、模块、依赖审计及打包统一见[开发指南](../docs/DEVELOPMENT.md)。首次安装与应用升级使用不同镜像，见[安装表](../docs/INSTALLATION.md#install)。内部测试构建不是公开发行版。

## 适配入口

- [硬件引脚与外设](../docs/HARDWARE.md)
- [交互与弹窗](../docs/USER-GUIDE.md)
- [摄像头协议与缩略图](CAMERA.md)
- [完整截图](../docs/SCREENSHOTS.md)

## 原厂资料与许可

[Waveshare S3](https://docs.waveshare.net/ESP32-S3-Touch-LCD-2.8/) · [Waveshare P4](https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-4.3/) · [P4 示例](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3)

面板初始化来源和许可保留在驱动注释。项目及依赖授权见[第三方说明](../LICENSE-3RD-PARTY.md)。
