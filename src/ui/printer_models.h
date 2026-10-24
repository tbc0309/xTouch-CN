#pragma once
#include <lvgl.h>
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void ui_printer_image_decoder_init(void);
void ui_printer_image_cache_begin(size_t budget);
void ui_printer_image_cache_end(void);
size_t ui_printer_image_cache_bytes(void);
const char *ui_printer_model_short_name(const char *value);
const char *ui_printer_model_code(const char *value);
const lv_img_dsc_t *ui_printer_model_image(const char *value);
bool ui_printer_model_chamber_heater(const char *value);
#ifdef __cplusplus
}
#endif
