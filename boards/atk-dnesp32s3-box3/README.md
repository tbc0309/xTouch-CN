# ALIENTEK／正点原子 ATK-DNESP32S3 BOX3

实物状态：➖ 未实测。适用下列准确主板和屏幕组合，不与其它同尺寸主板混刷。

![ALIENTEK／正点原子 ATK-DNESP32S3 BOX3](../../readme-assets/test-boards/atk-dnesp32s3-box3.png)

| 硬件 | 参数 |
| --- | --- |
| 显示 | 2.4英寸，ST7789V2，320×240 SPI |
| Flash / PSRAM | 16MB / 8MB PSRAM |
| 触摸 | CHSC5432，0x2E，AW9523 P1_7复位 |
| 声音 | ES8311 + NS4150B |
| 亮度 | AW9523 开关控制，不支持连续调光 |
| 状态灯 | 红蓝双色灯不作为RGB状态灯 |

固件包含八种屏幕语言、Wi-Fi扫码配网、网页管理、局域网/云端打印机、Token与账号密码/验证码登录、打印监控和机型支持的控制指令。硬件和打印机权限仍决定具体功能是否可用。

| 模块 | 配置 |
| --- | --- |
| 打印机摄像头 | JPEG及支持格式的H.264打印机画面 |
| 任务缩略图 | ✓ |
| 温湿度计（Xiaomi BLE） | ✓ |
| 待机 | 常规联网待机、触摸/BOOT唤醒；BOOT深睡唤醒待实物核验 |
| SD卡 | 接口暂未映射，不启用SD更新 |
| 蓝牙配网 | 默认不编译 |

激活ESP-IDF 6.1环境后，在项目根目录构建：

```sh
python boards/build.py atk-dnesp32s3-box3 --allow-candidate-build
```

应用与完整镜像生成于 `boards/build-atk-dnesp32s3-box3/artifacts/`。仅在确认主板匹配后使用原生串口工具刷写。测试固件保留独立项目身份及 main/test 双路OTA清单地址；编译结果不替代实物验收。

厂家资料：
- https://wiki.alientek.com/docs/Boards/IoT/DNESP32S3B3/example-idf/touch/
- https://wiki.alientek.com/docs/Boards/IoT/DNESP32S3B3/example-idf/iic_exio/
- https://wiki.alientek.com/docs/Boards/IoT/DNESP32S3B3/start-guide/dnesp32s3-box3-introduction/
