# Waveshare／微雪 ESP32-C5-Touch-LCD-3.5

实物状态：➖ 未实测。适用下列准确主板和屏幕组合，不与其它同尺寸主板混刷。

![Waveshare／微雪 ESP32-C5-Touch-LCD-3.5](../../readme-assets/test-boards/waveshare-c5-35.webp)

| 硬件 | 参数 |
| --- | --- |
| 显示 | 3.5英寸，ST7796，480×320（原生320×480） |
| Flash / PSRAM | 32MB / 8MB Quad PSRAM |
| 触摸 | FT6336；CH32V003复位 |
| 声音 | ES8311，BCLK时钟，无独立MCLK |
| 亮度 | CH32V003 8位PWM |
| 状态灯 | 无应用RGB状态灯 |

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
python boards/build.py waveshare-c5-35 --allow-candidate-build
```

应用与完整镜像生成于 `boards/build-waveshare-c5-35/artifacts/`。仅在确认主板匹配后使用原生串口工具刷写。测试固件保留独立项目身份及 main/test 双路OTA清单地址；编译结果不替代实物验收。

厂家资料：
- https://docs.waveshare.com/ESP32-C5-Touch-LCD-3.5
