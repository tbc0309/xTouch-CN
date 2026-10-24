# 共用界面图形资源

CYD、S3 小屏保留 `legacy/` 的原版 xTouch 字模；P4 大屏使用 `candidates/mdi/` 的调整版。主板仅选择资源尺寸与图标族，共用页面不复制实现。

## 图标

`mdi-map.json` 将原字形编码映射到 MDI。步距、Wi-Fi、摄像头采用 Lucide；轮廓以 1024×1024 画布中心归一化，普通图标最长边 900，竖尺高 960，腔体为 850×900。MDI 遵循 Apache-2.0，Lucide 遵循 ISC/MIT；原 xTouch 字模沿用原项目授权。

`original/` 与 `original-design/` 中的功能矢量重绘仅用于对比，不参与固件图标选择；并非上游原始矢量。

## Logo

`xperiments-logo.svg` 为 https://xperiments.in/ 首页指向 https://xtouch.pro 的内嵌 SVG，viewBox 203×46，保留原始贝塞尔路径。启动与桌面共用此来源，按屏幕尺寸生成，绿色 x 单独生成字形，避免裁切污染后面的字母。标识权利归原权利人。

运行 `python scripts/generate_ui_assets.py` 生成共用 Logo 与 P4 图标，并恢复小屏原字模。普通固件构建直接使用已生成资源，仅链接对应尺寸的一份。

## 打印机图

唯一输入集合为 `resources/printer-images/`，来源、分辨率及 SHA-256 见其中 `source.json`。`scripts/generate_printer_images.py` 输出 112px 小屏或 224px 大屏的行压缩 RGB565 图。缩放使用抗锯齿，255 色调色板；小屏与大屏均保留透明轮廓，解码可使用扫描线缓冲；S3/P4 设备选择页使用有上限的 PSRAM 图片缓存，关闭后释放，不加载全套图片到 RAM。

生成字体时须保留 SVG 轮廓实际左侧留白（hmtx 的 left side bearing），不能统一设为零，否则 FreeType 栅格化会产生水平偏移。界面按可见轮廓对齐，保持字形画布、字形留白和按钮内容区域各自的坐标一致。

设备来源标识使用 MDI `wifi`、`cloud-key-outline`、`cellphone`、`email-outline`（Apache-2.0），SVG 来自 https://github.com/Templarian/MaterialDesign/tree/master/svg 。网页与控制屏共用这些轮廓，`scripts/generate_source_icons.py` 生成 24px 小屏及 48px 大屏资源。

P4 大屏喷嘴与加热板沿用 MDI 原轮廓，仅调整三条热浪的位置与弯折（`source-icons/nozzle-heat-below.svg`、`source-icons/bed-heat-above.svg`）。`scripts/generate_toolhead_icons.py` 只更新大屏原生图标字形，小屏资源保持原样。

P4 打印头总成使用自绘单色 `source-icons/printhead-module.svg`，参考 Bambu Studio 总成模块结构。上述三枚图标按 MDI 字体 900/1024 可见轮廓范围统一归一化、居中，以原生像素生成；不改变小屏字形。
