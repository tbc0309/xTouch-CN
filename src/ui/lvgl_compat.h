#ifndef XTOUCH_LVGL_COMPAT_H
#define XTOUCH_LVGL_COMPAT_H
#ifndef __ASSEMBLER__
#include <lvgl.h>
#include "message_bus.h"
#include <string.h>
#include <stdlib.h>
#if defined(ESP_PLATFORM) && defined(XTOUCH_WAVESHARE)
#include <esp_heap_caps.h>
#endif
#include <src/draw/lv_draw_buf_private.h>
#include <src/misc/cache/instance/lv_image_cache.h>
#include <src/misc/cache/instance/lv_image_header_cache.h>
#include <src/misc/lv_area_private.h>
#include <src/misc/lv_text_private.h>
#if LVGL_VERSION_MAJOR >= 9
#define lv_mem_alloc lv_malloc
#define lv_mem_free lv_free
#define lv_mem_realloc lv_realloc
#define XTOUCH_FONT_NUMERIC (&ui_font_text_28)
#define _lv_txt_encoded_next lv_text_encoded_next
#define _lv_txt_get_encoded_length lv_text_get_encoded_length
#define lv_textarea_del_char lv_textarea_delete_char
static inline void lv_obj_set_style_anim_speed(lv_obj_t *o,uint32_t speed,lv_style_selector_t part){lv_obj_set_style_anim_duration(o,lv_anim_speed_clamped(speed,300,10000),part);}
#define lv_txt_get_size lv_text_get_size
#define LV_RES_OK LV_RESULT_OK
#define LV_RES_INV LV_RESULT_INVALID
#define LV_IMG_CF_ALPHA_8BIT LV_COLOR_FORMAT_A8
#define LV_IMG_CF_RAW LV_COLOR_FORMAT_RAW
#define LV_IMG_CF_TRUE_COLOR LV_COLOR_FORMAT_RGB565
#define LV_IMG_CF_TRUE_COLOR_ALPHA LV_COLOR_FORMAT_RGB565A8
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_SIZE 16
typedef lv_image_header_t lv_img_header_t;
typedef lv_image_decoder_t lv_img_decoder_t;
typedef lv_image_decoder_dsc_t lv_img_decoder_dsc_t;
typedef lv_layer_t lv_draw_ctx_t;

#define LV_IMG_SRC_SYMBOL LV_IMAGE_SRC_SYMBOL
#define LV_IMG_SRC_VARIABLE LV_IMAGE_SRC_VARIABLE
#define lv_event_get_draw_ctx lv_event_get_layer
#define _lv_area_intersect lv_area_intersect
#define lv_draw_img_dsc_init lv_draw_image_dsc_init
#define lv_img_decoder_get_info lv_image_decoder_get_info
#define lv_img_src_get_type lv_image_src_get_type
typedef lv_draw_image_dsc_t lv_draw_img_dsc_t;
#define lv_img_class lv_image_class
#define lv_color_to32 lv_color_to_u32
#define lv_obj_add_flag(o,f) lv_obj_add_flag((o),(lv_obj_flag_t)(f))
#define lv_obj_remove_flag(o,f) lv_obj_remove_flag((o),(lv_obj_flag_t)(f))
static inline lv_obj_t *ui_qrcode_create(lv_obj_t *parent,int32_t size,lv_color_t dark,lv_color_t light){lv_obj_t *o=lv_qrcode_create(parent);lv_qrcode_set_size(o,size);lv_qrcode_set_dark_color(o,dark);lv_qrcode_set_light_color(o,light);return o;}
/* Keep temporary font/layer pixels out of the fixed widget pool. */
static inline void *ui_font_tile_alloc(size_t bytes,lv_color_format_t format){(void)format;
#if defined(ESP_PLATFORM) && defined(XTOUCH_WAVESHARE)
 return heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
 return malloc(bytes);
#endif
}
static inline void ui_lvgl_draw_buffers_init(void){lv_draw_buf_handlers_t *h=lv_draw_buf_get_font_handlers();h->buf_malloc_cb=ui_font_tile_alloc;h->buf_free_cb=free;h=lv_draw_buf_get_handlers();h->buf_malloc_cb=ui_font_tile_alloc;h->buf_free_cb=free;}
static inline void lv_img_cache_invalidate_src(const void *src){lv_image_cache_drop(src);lv_image_header_cache_drop(src);}
static inline void ui_draw_label(lv_layer_t *layer,const lv_draw_label_dsc_t *style,const lv_area_t *area,const char *text,const void *hint){
 (void)hint;lv_draw_label_dsc_t d=*style;d.text=text;d.text_local=1;lv_draw_label(layer,&d,area);
}
/* Callers with 320x240 design coordinates opt in; native-pixel overlays do not. */
#define ui_draw_design_label ui_draw_label
static inline void ui_draw_line(lv_layer_t *layer,const lv_draw_line_dsc_t *style,const lv_point_t *p1,const lv_point_t *p2){
 lv_draw_line_dsc_t d=*style;d.p1.x=p1->x;d.p1.y=p1->y;d.p2.x=p2->x;d.p2.y=p2->y;lv_draw_line(layer,&d);
}
static inline void ui_draw_image(lv_layer_t *layer,const lv_draw_image_dsc_t *style,const lv_area_t *area,const void *src){
 lv_draw_image_dsc_t d=*style;d.src=src;lv_draw_image(layer,&d,area);
}
#endif

#endif /* !__ASSEMBLER__ */
#endif /* XTOUCH_LVGL_COMPAT_H */
