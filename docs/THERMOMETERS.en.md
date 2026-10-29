# Bluetooth thermometers

[简体中文](THERMOMETERS.md) | English | [Documentation](README.en.md)

Standard CYD, S3, P4 and ES3C35P firmware includes this module by default. Each display binds one sensor and identifies its model from the broadcast product ID. MiBeacon support does not mean every product in the Mi Home catalog is supported.

<a id="models"></a>
## Recognized identifiers

| Product | Model identifier | Product ID |
|---|---|---|
| 领普温湿度传感器  |  `KS2BB`  |  `0x520B` |
| 小米米家蓝牙温湿度计 2  |  `LYWSD03MMC`  |  `0x055B` |
| 米家智能温湿度计 3  |  `MJWSD05MMC`  |  `0x2832`, `0x4C47` |
| 米家智能温湿度计 3 mini  |  `MJWSD06MMC`  |  `0x55B5`, `0x5BEA` |
| 小米米家电子温湿度计 Pro  |  `LYWSD02MMC`  |  `0x16E4`, `0x2542` |
| 小米电子温湿度计  |  `XMWSDJ04MMC`  |  `0x1203` |
| 米家蓝牙温湿度计  |  `LYWSDCGQ`  |  `0x01AA` |
| 小米米家电子温湿度计 Pro  |  `LYWSD02`  |  `0x045B` |
| 青萍蓝牙温湿度计  |  `CGG1`  |  `0x0347`, `0x0B48` |
| 青萍蓝牙温湿度计 Lite  |  `CGDK2`  |  `0x066F` |
| 青萍温湿度计 Lite  |  `CGDK3`  |  `0x4F59` |
| 青萍蓝牙闹钟  |  `CGD1`  |  `0x0576` |
| 秒秒测时钟温湿度计  |  `MHO-C303`  |  `0x06D3` |
| 秒秒测蓝牙温湿度计  |  `MHO-C401`  |  `0x0387` |
| 青萍蓝牙小钟  |  `CGC1`  |  `0x0C3C` |
| 目博士温湿度计 MBS-17  |  `MBS17`  |  `0x5DB1` |
| 炎米智能温湿度计  |  `ESM787`  |  `0x78DB` |
| 青萍温湿度计  |  `CGG3`  |  `0x5808` |
| 米家智能温湿度计 3 Pro  |  `MJWSD07MMC`  |  `0x9AB9` |

There are 19 model identifiers and 23 product IDs. Product names follow Mi Home; other revisions with the same name may differ. This is a parser-support list, not a claim that every device was physically tested. Linp KS2 and Xiaomi thermometer 2 have physical evidence; the remaining entries use published protocol data and need device validation.

<a id="binding"></a>
## Binding

1. Open thermometer settings and enter the MAC.
2. Plain broadcasts can omit the BLE Key. Encrypted broadcasts need a 32-digit hexadecimal key; optional device-bound encryption protects its saved form.
3. Save and bind, then wait for temperature and humidity. Valid broadcasts populate Product (model identifier).
4. Select Hide, Alternate or Sensor only. Alternating defaults to 5 seconds, adjustable from 1–30 seconds.

The screen's Bluetooth thermometer card uses the same layout as Language and edits the same saved display preference as web management. It is disabled and gray until a sensor is bound, then becomes selectable without requiring a received measurement or a BLE Key for plain broadcasts. Removing the binding disables the dropdown automatically.

Unlocking a saved MAC shows a disabled save button; a valid change enables it. Changing MAC with a blank key clears the old device's key. Keeping the same MAC with a blank key retains its key. Missing keys are reported rather than borrowing another sensor's key.

<a id="ble-key"></a>

### Key extraction

Use [Xiaomi Cloud Tokens Extractor](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor): bind the device in Mi Home first, run its Release/Python program on a computer, sign in to the same Xiaomi account and region, and copy MAC and BLE KEY. The cloud TOKEN is not used.

```yaml
NAME:     领普温湿度传感器
ID:       blt.3.example
BLE KEY:  00112233445566778899aabbccddeeff
MAC:      02:86:B2:90:CE:4B
IP:       192.0.2.48
TOKEN:    00000000000000000000000000000000
MODEL:    linp.sensor_ht.ks2bb
```

These are fictional example values. Moving encrypted bindings to another display or resetting its keys may require key entry again.

<a id="compatibility"></a>
## Protocol limits

Supported table IDs use plain MiBeacon v2/v3 or AES-CCM v4/v5, with temperature/humidity objects `0x1004`/`0x1006`/`0x100D` and `0x4801`/`0x4802`/`0x4808`. Old v2/v3 encryption, unknown IDs, Zigbee, Wi-Fi, ATC, PVVX and BTHome are excluded. Clock devices provide temperature/humidity only, not alarms, time setting or CO₂ controls.


<a id="reporting"></a>
## Reception and display

Passive scanning listens for 30 ms in each 100 ms interval. It does not connect or request measurements. The sensor controls its broadcast interval and change threshold. Temperature uses one decimal; humidity fractions are truncated for integer percent display.

Temperature/humidity can arrive separately, with separate timestamps. Both must be valid before home display. Restarting or rebinding clears readings; changing the display mode/interval retains them. A value older than ten minutes adds `~`, indicating age rather than a confirmed fault.

Home checks readings once a second. Alternating changes presentation only; without AMS it shows environmental readings directly. Off hides readings but keeps reception active. Browser report ages refresh about every five seconds. Saving/refreshing cannot trigger a measurement; keep the sensor in range and wait for another broadcast.
