# 温湿度计支持列表

简体中文 | [English](THERMOMETERS.en.md) | [文档目录](README.md)

[支持型号](#models) · [绑定与密钥](#binding) · [兼容范围](#compatibility) · [采集与显示](#reporting)

<a id="models"></a>

S3、P4 发布固件默认包含 Xiaomi BLE 温湿度计模块；CYD 保留双 OTA 分区后，加入该模块会超出 Flash 应用容量，发行固件无法容纳。后台填写 MAC，以及加密广播所需的 BLE Key 后，固件会从广播的产品 ID 自动识别型号，无需手动选择；尚未收到广播时不会显示型号。

| 米家名称／产品名称 | 型号标识 | 产品 ID | 兼容说明 |
|---|---|---|---|
| 领普温湿度传感器 | `KS2BB` | `0x520B` | S3 可用 |
| 小米米家蓝牙温湿度计 2 | `LYWSD03MMC` | `0x055B` | S3、P4 可用 |
| 米家智能温湿度计 3 | `MJWSD05MMC` | `0x2832`, `0x4C47` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 米家智能温湿度计 3 mini | `MJWSD06MMC` | `0x55B5`, `0x5BEA` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 小米米家电子温湿度计 Pro | `LYWSD02MMC` | `0x16E4`, `0x2542` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 小米电子温湿度计 | `XMWSDJ04MMC` | `0x1203` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 米家蓝牙温湿度计 | `LYWSDCGQ` | `0x01AA` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 小米米家电子温湿度计 Pro | `LYWSD02` | `0x045B` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 青萍蓝牙温湿度计 | `CGG1` | `0x0347`, `0x0B48` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 青萍蓝牙温湿度计 Lite | `CGDK2` | `0x066F` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 青萍温湿度计 Lite | `CGDK3` | `0x4F59` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 青萍蓝牙闹钟 | `CGD1` | `0x0576` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 秒秒测时钟温湿度计 | `MHO-C303` | `0x06D3` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 秒秒测蓝牙温湿度计 | `MHO-C401` | `0x0387` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 青萍蓝牙小钟 | `CGC1` | `0x0C3C` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 目博士温湿度计 MBS-17 | `MBS17` | `0x5DB1` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 炎米智能温湿度计 | `ESM787` | `0x78DB` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 青萍温湿度计 | `CGG3` | `0x5808` | 按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |
| 米家智能温湿度计 3 Pro | `MJWSD07MMC` | `0x9AB9` | 仅温湿度；按对应 MiBeacon 产品 ID 识别；硬件版本与广播格式须一致 |

表内共 19 个型号标识、23 个产品 ID；同一商品的不同版本可能使用不同标识。青萍蓝牙闹钟（CGD1）、青萍蓝牙小钟（CGC1）、秒秒测时钟温湿度计（MHO-C303）是带温湿度传感器的时钟产品。支持的是温湿度广播读取，不包含时钟、闹钟或其它附加功能控制。

支持表中产品 ID 的 MiBeacon v2/v3 明文广播、v4/v5 AES-CCM 加密广播，以及 0x1004/0x1006/0x100D 和 0x4801/0x4802/0x4808 温湿度对象。明文广播可只填写 MAC；加密广播必须有正确 BLE Key。不支持 v2/v3 旧加密格式、未知产品 ID、其它服务 UUID 或全部 MiBeacon 设备。不包含改刷固件的 ATC、PVVX、BTHome 格式。相同商品名的其它硬件版本或广播格式不保证兼容。温度显示一位小数，所有型号的湿度统一舍去小数部分，以整数百分比显示。数值由传感器广播更新；读数不变时可能不立即重报。首页可选关闭显示、交替显示或仅温湿度，默认交替显示；通过滑块设置交替间隔（1～30 秒），默认 5 秒。`~` 表示较旧读数。

<a id="binding"></a>

## 绑定与获取 MAC、BLE Key

使用 [Xiaomi Cloud Tokens Extractor](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor)：

1. 在手机米家 App 中添加温湿度计，通过蓝牙完成设备绑定。
2. Windows 下载项目 Releases 中的程序，或按项目说明安装依赖并运行 Python 版本。
3. 登录绑定该温湿度计的小米账号，选择对应服务器地区，例如中国大陆（`cn`）。
4. 在设备列表中找到该温湿度计的 `MAC` 和 `BLE KEY`，填入后台温湿度计配置。明文型号没有 BLE Key 时可只填 MAC。

以下为格式示例，请填写自己的设备资料：

```yaml
NAME:     领普温湿度传感器
   ID:       blt.3.ee9d0ffd6159ce
   BLE KEY:  48449a24fb4394668c1b6a1d2cd5cfa3
   MAC:      02:86:B2:90:CE:4B
   IP:       192.0.2.48
   TOKEN:    67c7a9af1dd424a539997a4814043e03
   MODEL:    linp.sensor_ht.ks2bb
```

本功能只使用 `MAC` 和 `BLE KEY`，不需要 `ID`、`IP` 或云端 `TOKEN`。选择“加密并绑定”后，密钥绑定当前控制屏；在其它控制屏使用或重置设备密钥后需重新输入。

协议与型号参考：[Linp KS2](https://github.com/Tinnci/linpk2)、[Xiaomi BLE](https://github.com/Bluetooth-Devices/xiaomi-ble)、[ESPHome Xiaomi BLE](https://esphome.io/components/sensor/xiaomi_ble/)。

<a id="compatibility"></a>

## 兼容范围

米家 App 的“温湿度传感器”分类是商品目录，并不是 MiBeacon 协议兼容列表。本模块识别上表 19 个型号、23 个产品 ID；产品 ID 是广播中的型号编号，不是每台设备唯一的 `blt.…` ID。型号由广播自动识别，无需手工选择。

- 小米／Aqara 的 Zigbee 温湿度传感器不能由本模块直接接收。
- 标注 Wi-Fi 的 U-FAMILY U1、RSH Wi-Fi 等设备不属于本模块的 BLE 广播接入范围。
- LineHope／方舟鱼 TH1、TH1S、杜克 TH2、领普 KS1Pro、得力、MiKO、MBS-7／16／20、RSH、温度精灵、Anseny、HW 等目录商品，尚未核实到本模块可用的 MiBeacon 产品 ID 和温湿度报文，不列为已支持。相同外观或相近名称不能作为兼容依据。
- 米家智能温湿度计 3 Pro 只读取温湿度；不提供 CO₂、历史曲线、校时、屏幕控制或远程触发上报。
- 广播间隔和变化阈值由传感器决定。控制屏持续被动监听，不能保证按后台轮换间隔获得新数据；轮换间隔仅决定首页展示。

型号和报文核对来源：[Xiaomi BLE 型号表](https://github.com/Bluetooth-Devices/xiaomi-ble/blob/main/src/xiaomi_ble/devices.py)、[Passive BLE Monitor 解析器](https://github.com/custom-components/ble_monitor/blob/master/custom_components/ble_monitor/ble_parser/xiaomi.py)、[ESPHome Xiaomi BLE](https://esphome.io/components/sensor/xiaomi_ble/)、[领普 KS2](https://github.com/Tinnci/linpk2)。列表按产品 ID 与广播格式识别；同名产品的其它版本不保证兼容。

<a id="reporting"></a>

## 采集与显示

绑定后，控制屏被动监听广播，每 100 毫秒扫描周期监听 30 毫秒，不连接传感器，也不主动请求测量。上报周期和变化阈值由传感器决定，不能保证固定时间内收到新读数。

温度、湿度可能分开上报。只有绑定的传感器两项数据都收到后，首页才显示数值，并分别记录两项上报时间；密钥校验失败不会更新读数。重启或更换绑定会清空读数并等待新的广播；修改显示模式、交替间隔则保留已有读数。

首页每秒检查一次最新读数。“交替显示”按设定的 1～30 秒间隔轮换 AMS 与温湿度，默认 5 秒；没有 AMS 时直接显示温湿度。“仅温湿度”替代 AMS 展示；“关闭显示”只隐藏首页数值，仍继续接收。时间紧随摄像头图标，剩余宽度不足时隐藏。登录后台后，上报时间约每 5 秒刷新。

任一读数超过 10 分钟未收到更新时，保留上次读数并显示 `~`，表示收到的数据较旧，不代表已确认传感器故障。保存配置、刷新网页不会触发传感器重新上报；请保持传感器在附近，等待下一次测量广播。

更换 MAC 后，留空 BLE Key 保存会清除原设备密钥；不会将旧密钥用于新设备。相同 MAC 留空时保留已保存的密钥。明文广播只需 MAC；收到加密广播但缺少密钥时，后台提示需要 BLE Key。
