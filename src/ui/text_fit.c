#include "text_fit.h"
#include "native_fonts.h"
#include <string.h>

/* Native text sizes use their original glyph bitmaps. The shared scratch
 * buffer remains only for icons and text below the available native sizes;
 * it avoids offscreen layers in the small display's 32 KiB LVGL pool. */
typedef struct scaled_font {
    lv_font_t font;
    const lv_font_t *source;
    const lv_font_t *native;
    lv_obj_t *owner;
    unsigned zoom;
    struct scaled_font *next;
} scaled_font;
static scaled_font *fonts;
static uint8_t bitmap[2048];

static int scale(int n, unsigned z) { return n * (int)z / 256; }
static bool descriptor(const lv_font_t *font, lv_font_glyph_dsc_t *out,
                       uint32_t letter, uint32_t next)
{
    const scaled_font *s = font->dsc;
    if(!lv_font_get_glyph_dsc(s->source, out, letter, next)) return false;
    out->adv_w = scale(out->adv_w, s->zoom);
    out->box_w = (out->box_w * s->zoom + 255) / 256;
    out->box_h = (out->box_h * s->zoom + 255) / 256;
    out->ofs_x = scale(out->ofs_x, s->zoom);
    out->ofs_y = scale(out->ofs_y, s->zoom);
    out->bpp = 4;
    return true;
}
static unsigned pixel(const uint8_t *p, unsigned at, unsigned bpp)
{
    unsigned bit = at * bpp;
    return ((p[bit / 8] >> (8 - bpp - bit % 8)) & ((1u << bpp) - 1)) * 15 / ((1u << bpp) - 1);
}
static const uint8_t *glyph(const lv_font_t *font, uint32_t letter)
{
    const scaled_font *s = font->dsc;
    lv_font_glyph_dsc_t src, dst;
    if(!lv_font_get_glyph_dsc(s->source, &src, letter, 0)) return NULL;
    const uint8_t *input = lv_font_get_glyph_bitmap(src.resolved_font ? src.resolved_font : s->source, letter);
    if(!input || !src.box_w || !src.box_h) return NULL;
    descriptor(font, &dst, letter, 0);
    unsigned bytes = (dst.box_w * dst.box_h + 1) / 2;
    if(bytes > sizeof(bitmap) || (src.bpp != 1 && src.bpp != 2 && src.bpp != 4 && src.bpp != 8)) return NULL;
    memset(bitmap, 0, bytes);
    /* Area averaging retains thin strokes in small letters and icons. */
    for(unsigned y = 0; y < dst.box_h; ++y) {
        unsigned y0 = y * src.box_h / dst.box_h, y1 = (y + 1) * src.box_h / dst.box_h;
        if(y1 <= y0) y1 = y0 + 1;
        for(unsigned x = 0; x < dst.box_w; ++x) {
            unsigned x0 = x * src.box_w / dst.box_w, x1 = (x + 1) * src.box_w / dst.box_w, sum = 0;
            if(x1 <= x0) x1 = x0 + 1;
            for(unsigned yy = y0; yy < y1; ++yy)
                for(unsigned xx = x0; xx < x1; ++xx) sum += pixel(input, yy * src.box_w + xx, src.bpp);
            unsigned value = (sum + (x1-x0)*(y1-y0)/2) / ((x1-x0)*(y1-y0));
            unsigned at = y * dst.box_w + x;
            bitmap[at / 2] |= value << ((at & 1) ? 0 : 4);
        }
    }
    return bitmap;
}
static bool native_descriptor(const lv_font_t *font,lv_font_glyph_dsc_t *out,uint32_t letter,uint32_t next){
 const scaled_font *s=font->dsc;return lv_font_get_glyph_dsc(s->native,out,letter,next);
}
static const uint8_t *native_glyph(const lv_font_t *font,uint32_t letter){
 const scaled_font *s=font->dsc;lv_font_glyph_dsc_t d;
 if(!lv_font_get_glyph_dsc(s->native,&d,letter,0))return NULL;
 return lv_font_get_glyph_bitmap(d.resolved_font?d.resolved_font:s->native,letter);
}
bool ui_text_is_native(const lv_font_t *font){return font->get_glyph_dsc!=descriptor;}
const lv_font_t *ui_text_source(const lv_font_t *font)
{
    return (font->get_glyph_dsc == descriptor||font->get_glyph_dsc==native_descriptor) ? ((const scaled_font *)font->dsc)->source : font;
}
static void release(lv_event_t *e)
{
    scaled_font *s=lv_event_get_user_data(e), **at=&fonts;
    while(*at && *at!=s) at=&(*at)->next;
    if(*at) { *at=s->next; lv_mem_free(s); }
}
void ui_text_scale(lv_obj_t *label, const lv_font_t *source, unsigned zoom)
{
    source = ui_text_source(source);
    if(zoom >= 256) { if(lv_obj_get_style_text_font(label,0)!=source)lv_obj_set_style_text_font(label, source, 0); return; }
    const lv_font_t *native=ui_native_font_select(source,zoom,lv_label_get_text(label));
    scaled_font *s = NULL;
    for(scaled_font *at=fonts; at; at=at->next) if(at->owner == label) { s=at; break; }
    if(s&&s->source==source&&s->zoom==(zoom?zoom:1)&&s->native==native&&lv_obj_get_style_text_font(label,0)==&s->font)return;
    if(!s) {
        s=lv_mem_alloc(sizeof(*s));
        if(!s) return;
        s->owner=label; s->next=fonts; fonts=s;
        lv_obj_add_event_cb(label,release,LV_EVENT_DELETE,s);
    }
    if(!s) return;
    s->source=source; s->native=native; s->zoom=zoom ? zoom : 1; s->font=native?*native:*source;
    s->font.dsc=s; s->font.get_glyph_dsc=native?native_descriptor:descriptor; s->font.get_glyph_bitmap=native?native_glyph:glyph;
    if(!native){s->font.line_height=scale(source->line_height,s->zoom);s->font.base_line=scale(source->base_line,s->zoom);}
    s->font.fallback=NULL;
    lv_obj_set_style_text_font(label,&s->font,0);
}
void ui_text_guard(lv_obj_t *label)
{
    const lv_font_t *f=lv_obj_get_style_text_font(label,0);
    /* Reserve only real glyph overflow. Upright fonts need no slant margin. */
    uint32_t index=0; int left=0,right=0,top=0,bottom=0; const char *s=lv_label_get_text(label);
    while(s[index]) {
        uint32_t code=_lv_txt_encoded_next(s,&index); lv_font_glyph_dsc_t g;
        if(!lv_font_get_glyph_dsc(f,&g,code,0)) continue;
        if(-g.ofs_x>left) left=-g.ofs_x;
        if(g.ofs_x+g.box_w-g.adv_w>right) right=g.ofs_x+g.box_w-g.adv_w;
        int y=f->line_height-f->base_line-g.box_h-g.ofs_y;
        if(-y>top)top=-y;
        if(y+g.box_h-f->line_height>bottom)bottom=y+g.box_h-f->line_height;
    }
    if(lv_obj_get_style_pad_left(label,0)<left) lv_obj_set_style_pad_left(label,left,0);
    if(lv_obj_get_style_pad_right(label,0)<right) lv_obj_set_style_pad_right(label,right,0);
    if(lv_obj_get_style_pad_top(label,0)<top) lv_obj_set_style_pad_top(label,top,0);
    if(lv_obj_get_style_pad_bottom(label,0)<bottom) lv_obj_set_style_pad_bottom(label,bottom,0);
}
void ui_text_guard_tree(lv_obj_t *root)
{
    if(lv_obj_check_type(root,&lv_label_class) && !lv_obj_has_flag(root,UI_TEXT_EXACT_BOUNDS) && !lv_obj_has_flag(root,LV_OBJ_FLAG_CLICKABLE) && !lv_obj_has_flag(root,LV_OBJ_FLAG_IGNORE_LAYOUT)) {
        ui_text_guard(root);
    }
    for(unsigned i=0; i<lv_obj_get_child_cnt(root); ++i) ui_text_guard_tree(lv_obj_get_child(root,i));
}
