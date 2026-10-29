# esp32-s3-box-3

原生 BSP SPI 候选配置，尚未执行 IDF 编译或实物测试。

复用应用的唯一 LVGL 实例；官方 BSP 只创建显示、触摸与音频外设。温湿度计、云登录、摄像头及缩略图沿用共享模块。

深睡和 SD 尚未映射，已关闭；无独立 RGB 状态灯。

原始接口来源：https://github.com/espressif/esp-bsp/tree/f259a6abd5808019b1903a422092a4234ca95a51/bsp/esp-box-3
