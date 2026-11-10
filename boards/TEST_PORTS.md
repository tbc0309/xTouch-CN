# 测试版主板构建

测试版 `1.0.2-test` 提供 42 个新增主板与芯片修订目标，覆盖 ESP32、ESP32-C5、ESP32-C6、ESP32-S3、ESP32-S31 和 ESP32-P4。完整参数、固件配置及实测状态见[测试版主板列表](../README.test.md)。

## 目标配置

目标身份由 `board_catalog.py` 汇总，扩展配置位于 `port_targets.json`、`bsp_targets.json` 和对应板级目录。每个目标拥有独立项目身份、固件名和 OTA 路径；P4 芯片修订和屏幕控制器不同的镜像不可互刷。

`profile.cpp` 描述板级引脚和控制器，共用驱动位于 `common/port_*.cpp`。BSP 目标通过板级适配调用官方外设驱动，并使用应用的 LVGL 实例。

## 构建

激活 ESP-IDF 6.1 后，在仓库根目录执行：

```sh
python boards/build.py lcdwiki-es3c28p --jobs 4
python boards/build.py espressif-s3-box --allow-candidate-build
```

`--assets small|large` 选择资源档位，不改变显示接口或物理分辨率。产物位于 `boards/build-<目标>/artifacts/`；应用镜像用于 OTA，`-full.bin` 用于首次完整安装。

未映射的 SD、状态灯或深睡功能保持关闭。普通 BOOT 按键、联网待机与深睡唤醒是独立配置。已完成编译的固件不代表对应硬件已实测，列表中的 ➖ 状态表示尚无实物反馈。

## OTA

固件版本和发布标签为 `1.0.2-test`。OTA 探测对应板型的 `main`／`test` 清单，并校验项目身份、版本和摘要；同一数字版本优先正式版。预发布附件与正式版独立存放。Fork 的仓库与通道设置位于 `../resources/ota_channels.json`。

[主板移植接口](../docs/PORTING.md) · [开发与构建](../docs/DEVELOPMENT.md)
