# xTouch-CN

简体中文 | [English](README.en.md)

拓竹打印机的独立触摸控制屏。查看打印状态、AMS 和温湿度，操作温度、风扇与运动，并通过网页管理打印机和拓竹账号。非拓竹官方产品。

使用原生 ESP-IDF 6.1 与 LVGL 9.6.0。安装时请核对完整主板型号，并使用与固件版本对应的文档。

[安装指南](docs/INSTALLATION.md) · [在线安装](https://tbc0309.github.io/xTouch-CN/) · [固件下载](https://github.com/tbc0309/xTouch-CN/releases/latest) · [文档目录](docs/README.md)

## 功能

- 八种语言：简体中文、繁體中文、英语、法语、德语、俄语、日语、韩语。
- 查看打印状态、进度、温度、风扇、Wi-Fi信号与任务缩略图。
- 管理局域网打印机和云端账号，支持Token、账号密码与验证码登录。
- 暂停、恢复和取消打印，控制温度、风扇、灯光与运动。
- 查看AMS及外置耗材、材料颜色与已上报余量，操作设备支持的进退料、重读和烘干功能。
- 绑定BLE温湿度计，选择关闭显示、交替显示或仅温湿度。
- 查看支持的本地摄像头、故障码和打印机提示。
- 通过网页配置Wi-Fi、设备与账号，备份、恢复配置及升级固件。
- 设置语言、亮度、提示音和待机方式。

## 界面展示

以下六张为P4界面的演示数据渲染，摄像头画面仅作示意，不表示所有机型和外设均已实测。

| | |
|---|---|
| **首页**<br>![P4 首页](docs/images/boards/p4-home.png) | **打印状态**<br>![P4 打印状态](docs/images/boards/p4-printing.png) |
| **选择打印机**<br>![P4 选择打印机](docs/images/boards/p4-printer-picker.png) | **温度与风扇**<br>![P4 温度与风扇](docs/images/boards/p4-temperature.png) |
| **耗材**<br>![P4 耗材](docs/images/boards/p4-filament.png) | **摄像头**<br>![P4 摄像头](docs/images/boards/p4-camera.png) |

[查看完整界面图集](docs/SCREENSHOTS.md)

## 支持的主板

主板参数与功能支持分开列出。外观相似、芯片相同或屏幕尺寸相同都不代表镜像通用。

> **更多主板（测试版）**：[查看主板参数与实测状态](https://github.com/tbc0309/xTouch-CN/blob/test/README.test.md)。测试版以 Pre-release 提供，安装前请核对完整型号、查看实测状态并备份配置。

| 参数 | CYD | S3 2.8 V2 | P4 4.3 | ES3C35P 3.5 |
|---|---|---|---|---|
| 主板型号 | [ESP32-2432S028R（CYD）](docs/HARDWARE.md#cyd) | [Waveshare ESP32-S3-Touch-LCD-2.8 V2](docs/HARDWARE.md#waveshare) | [Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3](docs/HARDWARE.md#waveshare) | [LCDWIKI ES3C35P](boards/lcdwiki-es3c35p/README.md) |
| 图片 | <img src="docs/images/cyd-board-product.jpg" width="100%" alt="CYD主板"> | <img src="docs/images/boards/esp32-s3-touch-lcd-2.8-v2-product-right.svg" width="100%" alt="Waveshare S3 V2主板"> | <img src="docs/images/boards/esp32-p4-wifi6-touch-lcd-4.3-product-right.svg" width="100%" alt="Waveshare P4主板"> | <img src="docs/images/boards/es3c35p-product.jpg" width="100%" alt="LCDWIKI ES3C35P主板"> |
| 显示与触摸 | 320×240；ILI9341／XPT2046电阻触摸 | 320×240；ST7789／CST3530 | 800×480；ST7701／GT911 | 480×320；ST77922显示触摸一体，3.5寸 |
| 芯片与存储 | ESP32；4 MB Flash，无PSRAM | ESP32-S3；16 MB Flash、8 MB PSRAM | P4 v3.x＋C6；32 MB Flash、32 MB PSRAM | ESP32-S3；16 MB Flash、8 MB PSRAM；ES8311音频、RGB灯 |
| 默认资源 | 小屏 original；中英文字库、双OTA | 小屏 original | 大屏 MDI | 大屏 MDI；与S3相同功能模块 |

资源大小与图标风格为独立编译选项。各主板的硬件说明见板级文档；固件文件见下载包清单。[引脚、供电与外壳](docs/HARDWARE.md) · [打印机与AMS兼容](docs/PRINTERS.md)

### 各主板功能支持

✅ 支持；❌ 标准构建不提供；➖ 不适用。S3 指 Waveshare 2.8 V2，3.5寸指 LCDWIKI ES3C35P。功能是否可操作还取决于打印机、外设和固件权限。

| 功能 | CYD | S3 | P4 | 3.5寸 | 说明 |
|---|:---:|:---:|:---:|:---:|---|
| 设备／账号上限 | 20／10 | 100／50 | 100／50 | 100／50 | 实际还受可用内存及凭据长度限制 |
| 局域网设备 | ✅ | ✅ | ✅ | ✅ | IP、序列号与访问码验证后保存 |
| Access Token | ✅ | ✅ | ✅ | ✅ | 中国区／国际区账号；与密码登录独立编译 |
| 密码／验证码登录 | ✅ | ✅ | ✅ | ✅ | 按云端要求使用短信或邮箱验证 |
| 多打印机管理 | ✅ | ✅ | ✅ | ✅ | 一次连接一台；来源数据相互隔离 |
| 打印状态、暂停／恢复／取消 | ✅ | ✅ | ✅ | ✅ | 准备进度黄色，实际打印进度绿色 |
| 温度、风扇与运动控制 | ✅ | ✅ | ✅ | ✅ | 按机型能力显示；部分控制需要局域网＋开发者模式 |
| AMS／外置耗材 | ✅ | ✅ | ✅ | ✅ | 槽位、颜色、材料与已上报余量；支持的设备提供进退料等操作 |
| BLE 温湿度计 | ✅ | ✅ | ✅ | ✅ | [支持型号](docs/THERMOMETERS.md)；关闭显示／交替显示／仅温湿度 |
| 云端／局域网任务缩略图 | ❌ | ✅ | ✅ | ✅ | CYD受存储与运行内存限制，打印机型图使用绿色拓竹Logo |
| 本地JPEG摄像头 | ❌ | ✅ | ✅ | ✅ | 受机型协议限制；云设备仅摄像头可按同序列号关联本地连接 |
| 图片式设备选择 | ❌ | ✅ | ✅ | ✅ | CYD仅列表模式；其它板可切换图片／列表 |
| 故障码与必要提示 | ✅ | ✅ | ✅ | ✅ | 机型上报的HMS与打印错误 |
| 完整英文故障详情 | ❌ | ✅ | ✅ | ✅ | CYD容量受限，保留故障码与必要提示 |
| 屏幕与网页语言 | ✅ | ✅ | ✅ | ✅ | CYD中英，其它板八种屏幕语言；网页语言独立保存 |
| Wi-Fi扫码配网／网页管理 | ✅ | ✅ | ✅ | ✅ | 配置、备份与应用OTA升级 |
| 联网待机、亮度与提示音 | ✅ | ✅ | ✅ | ✅ | 保持联网；触摸唤醒，声音可关闭 |
| RGB 状态指示灯 | ✅ | ➖ | ➖ | ✅ | CYD / ES3C35P 板载 RGB；在线绿色、离线蓝色闪烁、告警绿黄交替；关闭开关或熄屏时关灯 |
| 蓝牙配网 | ❌ | ❌ | ❌ | ❌ | 标准构建关闭；定制条件见[构建选项](docs/DEVELOPMENT.md#options) |

## 3D 打印外壳

项目提供 [S3 与 P4 外壳模型](cases/README.md)，含按钮、不同接口开口及扬声器版本。按完整主板型号选择并解压 STL；CYD 与 ES3C35P 暂无随项目提供的模型。

## 开始使用

1. 核对完整主板型号，安装对应固件。应用镜像用于升级，`-full.bin`用于USB完整安装。
2. CYD首次启动按提示校准。扫描Wi-Fi配网二维码，连接设备热点并配置2.4 GHz网络。
3. 在屏幕设置中打开网页管理，使用初始密码`12345678`登录并修改密码。
4. 添加并验证局域网设备，或登录拓竹账号并勾选关联设备。

详细步骤、Token获取、配置备份与恢复见[安装指南](docs/INSTALLATION.md)。

## 使用边界

打印机能力与控制权限取决于型号、外设和固件。账号登录成功不代表所有控制命令获准；部分操作需要局域网模式和开发者模式。状态与任务图片按连接来源处理，云设备不会借用局域网设备的状态或任务资料。

联网待机可触摸唤醒；深睡唤醒取决于主板硬件，ES3C35P深睡使用BOOT键。配置备份包含连接凭据，请自行保管；管理后台用于可信局域网。[操作说明](docs/USER-GUIDE.md) · [备份与安全](docs/MAINTENANCE.md) · [故障码查询](https://tbc0309.github.io/xTouch-CN/errors.html)

## 开发与许可

四种目标均使用原生ESP-IDF。云HTTP和缩略图使用`esp_http_client`，MQTT使用`esp_mqtt_client`，不链接Arduino框架。共用界面、模块开关、构建和资源生成见[开发指南](docs/DEVELOPMENT.md)。

保留上游版权，遵循[项目许可证](LICENSE.md)；字体、图标和其他组件见[第三方许可](LICENSE-3RD-PARTY.md)。
