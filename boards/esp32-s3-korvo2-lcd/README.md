# esp32-s3-korvo2-lcd

原生 BSP SPI 候选配置，尚未执行 IDF 编译或实物测试。

复用应用的唯一 LVGL 实例；官方 BSP 只创建显示、触摸与音频外设。温湿度计、云登录、摄像头及缩略图沿用共享模块。

深睡和 SD 尚未映射，已关闭；无独立 RGB 状态灯。

Korvo 的 BOOT GPIO0 与 LCD 数据复用，不作为待机按键。调试使用板载 CP2102 UART；保留的 USB GPIO19/20 不视为已连接下载口。厂家资料确认 16 MB Flash / 8 MB PSRAM；尚未做本项目实物验证。

原始接口来源：https://github.com/espressif/esp-bsp/tree/f259a6abd5808019b1903a422092a4234ca95a51/bsp/esp32_s3_korvo_2
