# 资源生成

普通构建使用已生成资源，不需要下载原字体。开发生成需 Pillow、fontTools、Node.js 和固定版本的 lv_font_conv；具体参数以脚本 `--help` 为准。

- `scripts/generate_text_fonts.py`：统一文字子集及字重，包含摄像头／二维码标题与需要的故障说明字形。
- `scripts/generate_ui_assets.py`：图标和 Logo，资源输入及许可保存在 `resources/ui-assets/`。
- `scripts/generate_printer_images.py`：从 `resources/printer-images/` 生成各档机型图。
- `scripts/generate_cyd_resources.py`：CYD 中英文字库和单张打印 Logo。

S3／P4／ES3C35P 默认包含英文故障详情，CYD 不包含完整说明表。不要在板目录复制另一套字体；资源大小和图标风格是独立编译选项。OPPO Sans 600 与必要 Noto CJK 补字的授权见[第三方说明](../../LICENSE-3RD-PARTY.md)。

生成后检查字形覆盖、未使用资源、大小屏度量和最终 ELF；再运行所有支持主板的渲染矩阵及[文档检查](../../scripts/check_documentation.py)。构建入口见[开发指南](../../docs/DEVELOPMENT.md)。
