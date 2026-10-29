/* Bounded RGB565/RGB565A8 artwork decoding for LVGL 9. */
#include "printer_models.h"
#if XTOUCH_SINGLE_PRINTER_LOGO
void ui_printer_image_decoder_init(void){}
void ui_printer_image_cache_begin(size_t budget){(void)budget;}
void ui_printer_image_cache_end(void){}
size_t ui_printer_image_cache_bytes(void){return 0;}
#else
#include "layout_shared.h"
#include <src/image/lv_image_decoder_private.h>
#include <string.h>
#include <stdlib.h>
#if defined(ESP_PLATFORM) && defined(XTOUCH_WAVESHARE)
#include <esp_heap_caps.h>
#endif
static uint16_t read16(const uint8_t *p){return p[0]|((uint16_t)p[1]<<8);}
static const lv_img_dsc_t *packed_image(const void *src){
 if(lv_img_src_get_type(src)!=LV_IMG_SRC_VARIABLE)return NULL;
 const lv_img_dsc_t *image=src;
 if(image->header.cf!=LV_IMG_CF_RAW||!image->data||image->data_size<10)return NULL;
 const uint8_t *p=image->data;
 if((memcmp(p,"XTP1",4)&&memcmp(p,"XTP2",4))||!p[4]||!p[5]||!p[6])return NULL;
 unsigned table=8+p[6]*2+(p[5]+1)*2;
 return table<=image->data_size&&image->header.w==p[4]&&image->header.h==p[5]?image:NULL;
}
typedef struct CachedImage {const void *src;lv_draw_buf_t buffer;uint8_t *pixels;size_t bytes;uint32_t age;unsigned references;} CachedImage;
static CachedImage cache[6];static size_t cache_budget,cache_bytes;static uint32_t cache_clock;
static void discard(CachedImage *c){cache_bytes-=c->bytes;free(c->pixels);memset(c,0,sizeof(*c));}
void ui_printer_image_cache_end(void){cache_budget=0;for(unsigned i=0;i<6;i++)if(!cache[i].references)discard(&cache[i]);}
void ui_printer_image_cache_begin(size_t budget){ui_printer_image_cache_end();cache_budget=budget;}
size_t ui_printer_image_cache_bytes(void){return cache_bytes;}
static lv_result_t image_info(lv_image_decoder_t *decoder,lv_image_decoder_dsc_t *dsc,lv_image_header_t *header){
 (void)decoder;const lv_image_dsc_t *image=packed_image(dsc->src);if(!image)return LV_RESULT_INVALID;
 *header=image->header;header->magic=LV_IMAGE_HEADER_MAGIC;header->cf=image->data[3]=='2'?LV_COLOR_FORMAT_RGB565A8:LV_COLOR_FORMAT_RGB565;
 unsigned zoom=ui_layout_base_zoom();header->w=(header->w*zoom+127)/256;header->h=(header->h*zoom+127)/256;header->stride=header->w*2;return LV_RESULT_OK;
}
static lv_res_t image_read(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc,
                          lv_coord_t x, lv_coord_t y, lv_coord_t len, uint8_t *out)
{
    (void)decoder;
    const lv_img_dsc_t *image = dsc->user_data;
    if (!image || !out || x < 0 || y < 0 || len <= 0 || x+len > dsc->header.w || y >= dsc->header.h) return LV_RES_INV;
    unsigned sourceY=(unsigned)y*image->header.h/dsc->header.h;
    const uint8_t *p = image->data, *palette = p+8;
    unsigned stride=p[3]=='2'?3:2;
    const uint8_t *table=palette+p[6]*2;
    unsigned data_start = 8 + p[6]*2 + (p[5]+1)*2;
    unsigned start = data_start + read16(table+sourceY*2), end = data_start + read16(table+(sourceY+1)*2);
    if (end > image->data_size || start > end) return LV_RES_INV;
    if(stride==3){
        if(start+2>end)return LV_RES_INV;
        unsigned color_end=start+2+read16(p+start);start+=2;
        if(color_end>end)return LV_RES_INV;
        unsigned at=color_end,pixel=0,written=0;
        while(at<end&&written<(unsigned)len){
            unsigned token=p[at++],count=(token&127)+1;bool repeat=token&128;
            if(at+(repeat?1:count)>end||pixel+count>image->header.w)return LV_RES_INV;
            for(unsigned i=0;i<count;++i,++pixel){
                if(pixel==(unsigned)(x+written)*image->header.w/dsc->header.w){
                    out[written*3+2]=p[at+(repeat?0:i)];
                    if(++written==(unsigned)len)break;
                }
            }
            at+=repeat?1:count;
        }
        if(written!=(unsigned)len)return LV_RES_INV;
        end=color_end;
    }
    unsigned pixel = 0, written = 0;
    while (start < end && written < (unsigned)len) {
        unsigned token = p[start++], count = (token & 127)+1;
        bool repeat = token & 128;
        if (start + (repeat ? 1 : count) > end || pixel+count > image->header.w) return LV_RES_INV;
        for (unsigned i=0; i<count; ++i, ++pixel) {
            unsigned index = p[start+(repeat ? 0 : i)];
            if (index >= p[6]) return LV_RES_INV;
            if (pixel == (unsigned)(x+written)*image->header.w/dsc->header.w) {
                *out++ = palette[index*2]; *out++ = palette[index*2+1];
                if(stride==3)++out;
                ++written;
                if(written==(unsigned)len)break;
            }
        }
        start += repeat ? 1 : count;
    }
    return written == (unsigned)len ? LV_RES_OK : LV_RES_INV;
}

static lv_result_t image_open(lv_image_decoder_t *decoder,lv_image_decoder_dsc_t *dsc){
 if(image_info(decoder,dsc,&dsc->header)!=LV_RESULT_OK)return LV_RESULT_INVALID;
 CachedImage *c=NULL;
 for(unsigned i=0;i<6;i++)if(cache[i].src==dsc->src&&cache[i].buffer.header.w==dsc->header.w&&cache[i].buffer.header.h==dsc->header.h){c=&cache[i];break;}
 if(!c){
  for(unsigned i=0;i<6;i++)if(!cache[i].references&&(!c||!cache[i].pixels||cache[i].age<c->age)){c=&cache[i];if(!c->pixels)break;}
  if(!c)return LV_RESULT_INVALID;discard(c);
  unsigned w=dsc->header.w,h=dsc->header.h,stride=dsc->header.cf==LV_COLOR_FORMAT_RGB565A8?3:2;size_t bytes=(size_t)w*h*stride;
#if defined(ESP_PLATFORM) && defined(XTOUCH_WAVESHARE)
  uint8_t *pixels=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
  uint8_t *pixels=malloc(bytes);
#endif
  uint8_t *row=malloc(w*stride);if(!pixels||!row){free(pixels);free(row);return LV_RESULT_INVALID;}
  dsc->user_data=(void*)dsc->src;bool ok=true;
  for(unsigned y=0;y<h;y++){
   if(image_read(decoder,dsc,0,y,w,row)!=LV_RESULT_OK){ok=false;break;}
   if(stride==2)memcpy(pixels+(size_t)y*w*2,row,w*2);
   else for(unsigned x=0;x<w;x++){size_t at=(size_t)y*w+x;pixels[at*2]=row[x*3];pixels[at*2+1]=row[x*3+1];pixels[(size_t)w*h*2+at]=row[x*3+2];}
  }
  free(row);if(!ok){free(pixels);return LV_RESULT_INVALID;}
  lv_draw_buf_init(&c->buffer,w,h,(lv_color_format_t)dsc->header.cf,w*2,pixels,bytes);c->src=dsc->src;c->pixels=pixels;c->bytes=bytes;cache_bytes+=bytes;
 }
 c->age=++cache_clock;c->references++;dsc->decoded=&c->buffer;dsc->user_data=c;return LV_RESULT_OK;
}
static void image_close(lv_image_decoder_t *decoder,lv_image_decoder_dsc_t *dsc){
 (void)decoder;CachedImage *c=dsc->user_data;if(!c)return;if(c->references)c->references--;if(!c->references&&(!cache_budget||cache_bytes>cache_budget))discard(c);dsc->user_data=NULL;dsc->decoded=NULL;
}
void ui_printer_image_decoder_init(void){
 static lv_image_decoder_t *decoder;if(decoder)return;decoder=lv_image_decoder_create();if(!decoder)return;
 lv_image_decoder_set_info_cb(decoder,image_info);lv_image_decoder_set_open_cb(decoder,image_open);lv_image_decoder_set_close_cb(decoder,image_close);
}
#endif
