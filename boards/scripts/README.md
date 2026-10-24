# P4 素材生成

正常构建不需要运行这些脚本；生成结果已提交。开发时需要 Python 的 Pillow、fontTools，以及 Node.js/npx。字体转换器固定使用 lv_font_conv 1.5.3。

1. 大小屏文字子集统一由 `scripts/generate_text_fonts.py` 生成，使用通用字体文件名与 C 符号。
2. 图标与启动／桌面 Logo 统一由项目根目录 `scripts/generate_ui_assets.py` 生成，公共母版与大屏输出位于 `resources/ui-assets/`。
3. 机型图统一由 `scripts/generate_printer_images.py` 读取 `resources/printer-images/`；默认输出 112px 小屏版本，传入 `--size 224 --output resources/ui-assets/generated/large/printer_models.c` 输出大屏版本。

灯图标按卡片可用高度统一缩放；信号数字由共用布局定位，三板均只有负号覆盖 Wi-Fi 图标右上角，数字在图标外。

主板源码通过 `display_scale.h` 将 320×240 设计单位转换为原生画布坐标。适配层作用于应用页面、自绘、滚动及输入命中；P4 驱动直接提交原生像素。

## 可选错误码详情

S3/P4 默认 `XTOUCH_ERROR_DETAILS=OFF`：显示错误码、HMS 和通用提示，不包含中英文详情表与详情专用扩展字形。CYD 不提供内置详情。

需要内置详情时，在对应 IDF 构建中传入 `-DXTOUCH_ERROR_DETAILS=ON`；改回 `OFF` 后重新构建即可移除。P4 的精简字体位于 `assets/fonts/compact/`，完整字体位于 `assets/fonts/`；S3 仅开启该选项时使用其扩展字体。

字库生成脚本统一更新正文、数字、二维码标题、摄像头和可选错误码详情字库。图标与 Logo 独立生成。

界面文字统一使用 OPPO Sans 4.0，字重 600。大小屏文字子集由 `scripts/generate_text_fonts.py` 生成，正常构建无需运行生成脚本。日文假名和韩文由 Noto Sans CJK 补齐。
