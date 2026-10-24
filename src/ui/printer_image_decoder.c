/* Row-addressable printer artwork. See scripts/generate_printer_images.py.
 * Flash scanlines are the fallback; PSRAM boards may cache a bounded set
 * of decoded images while the device picker is open.
 */
#include "printer_models.h"
#include "layout_shared.h"
#include <string.h>
#include <stdlib.h>
#if defined(ESP_PLATFORM) && defined(XTOUCH_WAVESHARE)
#include <esp_heap_caps.h>
#endif
static struct {const void *src;uint8_t *pixels;size_t bytes;uint32_t age;uint16_t w,h;} cache[6];
static size_t cache_budget,cache_bytes;
static uint32_t cache_clock;
void ui_printer_image_cache_end(void){
    for(unsigned i=0;i<6;++i){free(cache[i].pixels);memset(&cache[i],0,sizeof(cache[i]));}
    cache_budget=cache_bytes=cache_clock=0;
}
void ui_printer_image_cache_begin(size_t budget){ui_printer_image_cache_end();cache_budget=budget;}
size_t ui_printer_image_cache_bytes(void){return cache_bytes;}
static lv_res_t image_read(lv_img_decoder_t *,lv_img_decoder_dsc_t *,lv_coord_t,lv_coord_t,lv_coord_t,uint8_t *);
static uint8_t *cached_pixels(lv_img_decoder_t *decoder,lv_img_decoder_dsc_t *dsc){
    if(!cache_budget)return NULL;
    unsigned stride=dsc->header.cf==LV_IMG_CF_TRUE_COLOR_ALPHA?3:2;
    size_t bytes=(size_t)dsc->header.w*dsc->header.h*stride;
    if(bytes>cache_budget)return NULL;
    for(unsigned i=0;i<6;++i)if(cache[i].src==dsc->src&&cache[i].w==dsc->header.w&&cache[i].h==dsc->header.h){cache[i].age=++cache_clock;return cache[i].pixels;}
    unsigned slot=0;
    for(unsigned i=0;i<6;++i)if(!cache[i].pixels||cache[i].age<cache[slot].age)slot=i;
    while(cache_bytes+bytes>cache_budget&&cache_bytes){
        unsigned oldest=slot;
        for(unsigned i=0;i<6;++i)if(cache[i].pixels&&(!cache[oldest].pixels||cache[i].age<cache[oldest].age))oldest=i;
        cache_bytes-=cache[oldest].bytes;free(cache[oldest].pixels);memset(&cache[oldest],0,sizeof(cache[oldest]));
    }
    if(cache[slot].pixels){cache_bytes-=cache[slot].bytes;free(cache[slot].pixels);memset(&cache[slot],0,sizeof(cache[slot]));}
#if defined(ESP_PLATFORM) && defined(XTOUCH_WAVESHARE)
    uint8_t *pixels=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    uint8_t *pixels=malloc(bytes);
#endif
    if(!pixels)return NULL;
    for(unsigned y=0;y<dsc->header.h;++y)if(image_read(decoder,dsc,0,y,dsc->header.w,pixels+(size_t)y*dsc->header.w*stride)!=LV_RES_OK){free(pixels);return NULL;}
    cache[slot].src=dsc->src;cache[slot].pixels=pixels;cache[slot].bytes=bytes;cache[slot].w=dsc->header.w;cache[slot].h=dsc->header.h;cache[slot].age=++cache_clock;cache_bytes+=bytes;
    return pixels;
}

#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP
#error Printer artwork requires the configured native RGB565 color layout
#endif

static uint16_t read16(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }

static const lv_img_dsc_t *packed_image(const void *src)
{
    if (lv_img_src_get_type(src) != LV_IMG_SRC_VARIABLE) return NULL;
    const lv_img_dsc_t *image = src;
    if (image->header.cf != LV_IMG_CF_RAW || !image->data || image->data_size < 10) return NULL;
    const uint8_t *p = image->data;
    if ((memcmp(p,"XTP1",4)&&memcmp(p,"XTP2",4)) || !p[4] || !p[5] || !p[6]) return NULL;
    unsigned table_end = 8 + p[6]*2 + (p[5]+1)*2;
    if (table_end > image->data_size || image->header.w != p[4] || image->header.h != p[5]) return NULL;
    return image;
}

static lv_res_t image_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    (void)decoder;
    const lv_img_dsc_t *image = packed_image(src);
    if (!image) return LV_RES_INV;
    *header = image->header;
    header->cf = image->data[3]=='2'?LV_IMG_CF_TRUE_COLOR_ALPHA:LV_IMG_CF_TRUE_COLOR;
    unsigned zoom=ui_layout_base_zoom();
    header->w=(header->w*zoom+127)/256;header->h=(header->h*zoom+127)/256;
    return LV_RES_OK;
}

static lv_res_t image_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    if (image_info(decoder, dsc->src, &dsc->header) != LV_RES_OK) return LV_RES_INV;
    dsc->user_data = (void *)dsc->src;
    dsc->img_data = cached_pixels(decoder,dsc);
    return LV_RES_OK;
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

void ui_printer_image_decoder_init(void)
{
    static lv_img_decoder_t *decoder;
    if (decoder) return;
    decoder = lv_img_decoder_create();
    if (!decoder) return;
    lv_img_decoder_set_info_cb(decoder, image_info);
    lv_img_decoder_set_open_cb(decoder, image_open);
    lv_img_decoder_set_read_line_cb(decoder, image_read);
}
