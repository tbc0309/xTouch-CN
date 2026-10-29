#pragma once
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_ek79007.h>
inline esp_err_t portCreatePanel(esp_lcd_dsi_bus_handle_t bus,esp_lcd_panel_io_handle_t io,esp_lcd_panel_handle_t *panel){
esp_lcd_dpi_panel_config_t dpi{};dpi.dpi_clk_src=MIPI_DSI_DPI_CLK_SRC_DEFAULT;dpi.in_color_format=LCD_COLOR_FMT_RGB565;dpi.out_color_format=LCD_COLOR_FMT_RGB565;
dpi.dpi_clock_freq_mhz=52;dpi.video_timing.h_size=1024;dpi.video_timing.v_size=600;
dpi.video_timing.hsync_pulse_width=10;
dpi.video_timing.hsync_back_porch=160;
dpi.video_timing.hsync_front_porch=160;
dpi.video_timing.vsync_pulse_width=1;
dpi.video_timing.vsync_back_porch=23;
dpi.video_timing.vsync_front_porch=12;
dpi.num_fbs=2;
ek79007_vendor_config_t vendor{};vendor.mipi_config.dsi_bus=bus;vendor.mipi_config.dpi_config=&dpi;
esp_lcd_panel_dev_config_t cfg{};cfg.reset_gpio_num=gpio_num_t(33);cfg.rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB;cfg.bits_per_pixel=16;cfg.vendor_config=&vendor;
return esp_lcd_new_panel_ek79007(io,&cfg,panel);
}
