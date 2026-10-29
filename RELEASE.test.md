# xTouch-CN 1.0.1-test · 新主板测试版

基于 xTouch-CN 1.0.1，提供 38 个新增主板／芯片修订固件目标，覆盖 ESP32、ESP32-S3 和 ESP32-P4。使用原生 ESP-IDF 6.1 与 LVGL 9.6.0。

本次为 **Pre-release**。附件固件已完成原生编译、镜像与 ELF 对应、Flash／双 OTA 容量、资源配置及 OTA 身份检查。新增板型的实物验收状态独立记录，尚未实测的项目标为 **➖ 未实测**。

## 主板与功能

- [主板参数、固件配置与实测状态](https://github.com/tbc0309/xTouch-CN/blob/test/README.test.md)
- 支持局域网和云端打印监测、Token 与账号／验证码登录、温湿度计，以及硬件和打印机支持的控制功能。
- 按屏幕分辨率选择资源规格与图标；语言、状态灯、深睡等模块配置见主板列表。
- 经典 ESP32 测试目标的摄像头仅提供兼容 JPEG；未映射的 SD、状态灯或独立深睡唤醒不启用。
- 资料或容量尚未满足条件的型号不提供本次安装镜像。现有四款正式主板继续使用 `1.0.1` 正式版。

## 安装

在[在线刷机页面](https://tbc0309.github.io/xTouch-CN/)勾选右上角 **测试版**，核对完整主板型号、屏幕、Flash／PSRAM 和 P4 芯片修订后安装。测试版卡片以红框提示，安装前请备份配置。

手动下载时：`*-full.bin` 为对应主板的完整刷机包，`*.bin` 为应用／OTA 镜像。请勿将其它主板的镜像互刷。在线安装按分段保护 NVS；勾选擦除设备仍会删除设置。

测试固件自身版本、发布标签和 OTA 版本均为 `1.0.1-test`。固件探测对应板型的 `main`／`test` OTA 清单，校验后选择较新版本；同一数字版本的正式版优先。正式版与预发布附件独立存放。

附件包括各目标的应用及完整镜像、`test-firmware-catalog.json` 和 `SHA256SUMS.txt`。

## 反馈

请在[Issues](https://github.com/tbc0309/xTouch-CN/issues)中注明完整主板型号、屏幕／触摸组合、固件版本和复现步骤，并反馈显示、触摸、声音、联网、温湿度计与待机的实际表现。请勿公开账号、Token、访问码或 Wi-Fi 密码。

---

**Test firmware:** 38 additional board/silicon targets based on 1.0.1. Native build, image/ELF, resource, partition and OTA identity checks are complete; physical validation is tracked separately in the board catalog. Select **Test firmware** in the web installer and match the exact hardware. Existing release boards continue to use stable 1.0.1. Report hardware details and reproduction steps without credentials.
