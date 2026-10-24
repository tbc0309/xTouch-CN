# 摄像头与任务缩略图 / Camera and job thumbnails

[中文说明](#zh) · [English](#en)

<a id="zh"></a>

## 摄像头查看

S3 V2 和 P4 支持摄像头；ESP32-2432S028R 不提供该功能。点击首页左上角的摄像头区域打开，点击右上角关闭。

底部按钮为仓灯、暂停／恢复画面和重新连接。正常播放 4 秒后工具栏隐藏，点按画面即可唤出。暂停只冻结本地画面，不影响打印、录像或延时摄影。正常接收画面时保持亮屏；连接失败或无画面时仍受休眠设置控制。

### 首页状态图标

打印机在线且报告有摄像头时显示图标；此状态显示适用于三种主板，视频查看仍受主板能力和本地连接条件限制。

| 状态 | 含义 |
|---|---|
| 白色 | 录像、延时摄影均关闭 |
| 绿色常亮 | 监控录像已启用，延时摄影未启用 |
| 绿色闪烁 | 延时摄影已启用 |
| 右上角蓝点 | 可以点击查看视频，与录像、延时摄影状态独立 |

录像指示读取打印机报告的监控录像开关状态，不表示此刻是否正在写入录像文件。延时摄影表示功能已启用。断线、重连或切换设备后，摄像头状态重新获取。能否点击由视频连接条件决定，蓝点不随延时摄影图标闪烁。

### 连接条件

- 控制屏和打印机位于同一局域网，需要打印机的 IPv4 地址和 8 位访问码。
- 使用拓竹账号设备时，需先保存相同序列号的局域网设备，摄像头默认自动关联同一序列号的有效本地连接；不提供手动关联选项；若同序列号存在地址或访问码冲突的重复记录，请删除或修正重复记录。此关联仅用于摄像头；Token 不能替代摄像头访问码。仅保存云端设备而没有本地视频连接时，摄像头入口不可用。
- 摄像头被其他客户端占用、认证失败或端口不可用时，可关闭其他观看端后重连。

| 机型 | 视频协议 | 兼容情况 |
|---|---|---|
| P1P / P1S | 本地 TLS 6000 JPEG | 支持，P1S 已验证 |
| A1 / A1 mini | 本地 TLS 6000 JPEG | 支持该协议，实机兼容性未确认 |
| X1 / X1 Carbon / X1E、P2S / X2D、H2 系列 | RTSPS 322 H.264 | 支持兼容的 Baseline 码流，实机编码兼容性未确认 |
| A2L / 未知型号 | 未支持的协议 | 不提供连接 |

H.264 使用软件解码，支持 progressive Baseline / I420，不支持 Main、High、CABAC 和交错扫描。S3 的 8 MB PSRAM 限制可播放分辨率与参考帧数量；P4 有更大内存余量。缩小屏幕画面不能降低源视频参考帧的内存需求。不兼容编码或内存不足时显示错误并保留退出操作。

画面等比显示，不拉伸或裁切。JPEG 最大源尺寸 1920×1080，单帧最大 1 MiB。RTSPS 不提供云端视频、音频、录像保存或数字缩放。

### 任务缩略图

S3/P4 获取独立 PNG。云端模式仅使用当前设备所属拓竹账号及其区域的任务图片。打印机未上报盘号时，通过同一设备、同一任务的云端记录确定盘号；已上报的盘号必须一致。局域网模式仅访问当前局域网设备，不使用其他配置中的账号或云端图片；支持独立 PNG，以及当前任务 `.3mf` / `.gcode.3mf` 中指定盘的 PNG；兼容存储、Deflate、数据描述符和 ZIP64 格式，最多读取前 1 MiB，取图完成即停止。没有可用图片时显示机型图。PNG 最大 256 KiB、512×512，S3 显示尺寸最大 128 像素，P4 最大 512 像素。图片保存在内存中，任务结束或切换设备时清除，不写入 SD。

<a id="en"></a>

## Camera viewing

Camera viewing is available on S3 V2 and P4, not ESP32-2432S028R. Tap the camera area at the top left of Home to open it; use the top-right close button to exit.

Bottom controls toggle the chamber light, pause/resume the local picture and reconnect. The toolbar hides after four seconds of normal playback; tap the image to show it. Pausing does not affect printing, recording or timelapse. Incoming video keeps the screen awake; failed connections or missing frames remain subject to standby settings.

### Home status icon

The icon appears when the printer is online and reports a camera. All three boards share this indicator; video viewing still depends on board support and local connection details.

| State | Meaning |
|---|---|
| White | Recording and timelapse are off |
| Steady green | Monitoring recording is enabled; timelapse is off |
| Blinking green | Timelapse is enabled |
| Blue dot at the top right | Tap to view video; independent of recording and timelapse |

Recording indication follows the reported recording setting, rather than whether a recording file is being written at that moment. Timelapse indicates the enabled setting. Camera state is reacquired after disconnection, reconnection or a printer change. Video connection requirements determine whether the icon can be tapped. The blue dot does not blink with the timelapse indicator.

### Connection requirements

- The screen and printer must share a LAN. Supply the printer's IPv4 address and eight-character access code.
- For a Bambu-account device, save a LAN device with the same serial number. The camera automatically links a unique valid local connection for that serial number. There is no manual link setting. If duplicate records for the same serial have conflicting addresses or access codes, remove or correct the duplicates. This link is used only for the camera; a Token does not replace its access code. The camera entry is unavailable when only a cloud device is saved, with no local video connection.
- If another viewer occupies the camera, authentication fails or the port is unavailable, close other viewers and reconnect.

| Models | Protocol | Compatibility |
|---|---|---|
| P1P / P1S | Local TLS 6000 JPEG | Supported; P1S verified |
| A1 / A1 mini | Local TLS 6000 JPEG | Protocol supported; physical compatibility unconfirmed |
| X1 / X1 Carbon / X1E, P2S / X2D, H2 series | RTSPS 322 H.264 | Compatible Baseline streams supported; printer codec compatibility unconfirmed |
| A2L / unknown models | Unsupported protocol | Connection unavailable |

H.264 uses software decoding for progressive Baseline / I420. Main, High, CABAC and interlaced video are unsupported. S3's 8 MB PSRAM limits source resolution and reference frames; P4 has more memory available. Reducing display size does not reduce source reference-frame memory. Unsupported encoding or insufficient memory shows an error while keeping exit available.

Video retains its aspect ratio without stretching or cropping. JPEG sources are limited to 1920×1080 and 1 MiB per frame. RTSPS does not provide cloud video, audio, recording or digital zoom.

### Job thumbnails

S3/P4 fetch separate PNG images. Cloud mode uses only the selected device’s Bambu account and region. If the printer omits the plate number, it is resolved from the exact device and task record; an explicitly reported plate must match. LAN mode accesses only the selected LAN device and never borrows cloud accounts or covers from other profiles. Local extraction supports standalone PNG paths and the selected plate PNG inside the current `.3mf` / `.gcode.3mf` task. Stored, Deflate, data descriptor and ZIP64 formats are supported within a 1 MiB prefix; transfer stops once extraction completes. Printer artwork appears when no image is available. PNG limits are 256 KiB and 512×512; displayed sizes are up to 128 pixels on S3 and 512 on P4. Images remain in RAM and are cleared when a job ends or the printer changes; they are not saved to SD.
