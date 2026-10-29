# Espressif／乐鑫 ESP32-S31-Korvo-1 V1.1

实物状态：➖ 未实测。适用下列准确主板和屏幕组合，不与其它同尺寸主板混刷。

![Espressif／乐鑫 ESP32-S31-Korvo-1 V1.1](../../readme-assets/test-boards/esp32-s31-korvo-1.png)

| 硬件 | 参数 |
| --- | --- |
| 显示 | 4.3英寸，800×480 RGB；匹配ESP32-S3-LCD-EV-Board-SUB3子板 |
| Flash / PSRAM | 16MB / 16MB Octal PSRAM |
| 触摸 | GT1151，I2C轮询 |
| 声音 | ES8389，0x20，双功放 |
| 亮度 | 硬件背光常开；通过LCD使能切换显示 |
| 状态灯 | WS2812，GPIO37 |

固件包含八种屏幕语言、Wi-Fi扫码配网、网页管理、局域网/云端打印机、Token与账号密码/验证码登录、打印监控和机型支持的控制指令。硬件和打印机权限仍决定具体功能是否可用。

| 模块 | 配置 |
| --- | --- |
| 打印机摄像头 | JPEG画面；不包含S3/P4专用H.264解码器 |
| 任务缩略图 | ✓ |
| 温湿度计（Xiaomi BLE） | ✓ |
| 待机 | 常规联网待机，触摸/BOOT唤醒；不包含深睡 |
| SD卡 | 接口暂未映射，不启用SD更新 |
| 蓝牙配网 | 默认不编译 |

板载本机摄像头不接入本固件；此处摄像头功能指查看打印机画面。

激活ESP-IDF 6.1环境后，在项目根目录构建：

```sh
python boards/build.py esp32-s31-korvo-1 --allow-candidate-build
```

应用与完整镜像生成于 `boards/build-esp32-s31-korvo-1/artifacts/`。仅在确认主板匹配后使用原生串口工具刷写。测试固件保留独立项目身份及 main/test 双路OTA清单地址；编译结果不替代实物验收。

厂家资料：
- https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s31/esp32-s31-korvo-1/index.html
