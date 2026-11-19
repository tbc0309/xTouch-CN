#include "text_fit.h"
#include "native_fonts.h"
#include <string.h>

/* Native text sizes use their original glyph bitmaps. The shared scratch
 * buffer remains only for icons and text below the available native sizes;
 * it avoids offscreen layers in the small display's 48 KiB LVGL pool. */
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
    out->format = LV_FONT_GLYPH_FORMAT_A4;
    out->gid.index = letter;
    return true;
}
static const void *glyph(lv_font_glyph_dsc_t *d, lv_draw_buf_t *output)
{
    const scaled_font *s=d->resolved_font->dsc;
    lv_font_glyph_dsc_t src;
    if(!lv_font_get_glyph_dsc(s->source,&src,d->gid.index,0)||!src.box_w||!src.box_h)return NULL;
    lv_draw_buf_t *input=lv_draw_buf_create(src.box_w,src.box_h,LV_COLOR_FORMAT_A8,LV_STRIDE_AUTO);
    if(!input)return NULL;
    const lv_draw_buf_t *decoded=lv_font_get_glyph_bitmap(&src,input);
    if(!decoded){lv_draw_buf_destroy(input);return NULL;}
    unsigned bytes=(d->box_w*d->box_h+1)/2;
    if(d->req_raw_bitmap&&bytes>sizeof(bitmap)){lv_draw_buf_destroy(input);return NULL;}
    if(d->req_raw_bitmap)memset(bitmap,0,bytes);
    for(unsigned y=0;y<d->box_h;y++)for(unsigned x=0;x<d->box_w;x++){
        unsigned x0=x*src.box_w/d->box_w,x1=(x+1)*src.box_w/d->box_w;
        unsigned y0=y*src.box_h/d->box_h,y1=(y+1)*src.box_h/d->box_h,sum=0;
        if(x1<=x0)x1=x0+1;if(y1<=y0)y1=y0+1;
        for(unsigned yy=y0;yy<y1;yy++)for(unsigned xx=x0;xx<x1;xx++)sum+=decoded->data[yy*decoded->header.stride+xx];
        unsigned count=(x1-x0)*(y1-y0),value=(sum+count/2)/count;
        if(d->req_raw_bitmap){unsigned at=y*d->box_w+x;bitmap[at/2]|=(value/17)<<((at&1)?0:4);}
        else output->data[y*output->header.stride+x]=value;
    }
    lv_font_glyph_release_draw_data(&src);lv_draw_buf_destroy(input);
    return d->req_raw_bitmap?(const void*)bitmap:(const void*)output;
}
static bool native_descriptor(const lv_font_t *font,lv_font_glyph_dsc_t *out,uint32_t letter,uint32_t next){
 const scaled_font *s=font->dsc;if(!lv_font_get_glyph_dsc(s->native,out,letter,next))return false;out->gid.index=letter;return true;
}
static const void *native_glyph(lv_font_glyph_dsc_t *d,lv_draw_buf_t *output){
 const scaled_font *s=d->resolved_font->dsc;lv_font_glyph_dsc_t native;
 if(!lv_font_get_glyph_dsc(s->native,&native,d->gid.index,0))return NULL;native.req_raw_bitmap=d->req_raw_bitmap;
 return native.resolved_font->get_glyph_bitmap(&native,output);
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
    if(zoom >= 256) { if(lv_obj_get_style_text_font(label,(lv_part_t)(0))!=source)lv_obj_set_style_text_font(label, source, 0); return; }
    /* Fonts also belong to containers and are inherited by their children.
     * Reading the label extension of a plain object corrupts this path on
     * intermediate canvases, where the default font needs scaling too. */
    const char *text=lv_obj_check_type(label,&lv_label_class)?lv_label_get_text(label):"";
    const lv_font_t *native=ui_native_font_select(source,zoom,text);
    scaled_font *s = NULL;
    for(scaled_font *at=fonts; at; at=at->next) if(at->owner == label) { s=at; break; }
    if(s&&s->source==source&&s->zoom==(zoom?zoom:1)&&s->native==native&&lv_obj_get_style_text_font(label,(lv_part_t)(0))==&s->font)return;
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
    s->font.fallback=NULL; s->font.static_bitmap=0;
    lv_obj_set_style_text_font(label,&s->font,0);
}
void ui_text_guard(lv_obj_t *label)
{
    const lv_font_t *f=lv_obj_get_style_text_font(label,(lv_part_t)(0));
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
    if(lv_obj_get_style_pad_left(label,(lv_part_t)(0))<left) lv_obj_set_style_pad_left(label,left,0);
    if(lv_obj_get_style_pad_right(label,(lv_part_t)(0))<right) lv_obj_set_style_pad_right(label,right,0);
    if(lv_obj_get_style_pad_top(label,(lv_part_t)(0))<top) lv_obj_set_style_pad_top(label,top,0);
    if(lv_obj_get_style_pad_bottom(label,(lv_part_t)(0))<bottom) lv_obj_set_style_pad_bottom(label,bottom,0);
}
void ui_text_guard_tree(lv_obj_t *root)
{
    if(lv_obj_check_type(root,&lv_label_class) && !lv_obj_has_flag(root,UI_TEXT_EXACT_BOUNDS) && !lv_obj_has_flag(root,LV_OBJ_FLAG_CLICKABLE) && !lv_obj_has_flag(root,LV_OBJ_FLAG_IGNORE_LAYOUT)) {
        ui_text_guard(root);
    }
    for(unsigned i=0; i<lv_obj_get_child_cnt(root); ++i) ui_text_guard_tree(lv_obj_get_child(root,i));
}


#include "symbol_fonts.h"


void ui_text_variable_caption(lv_obj_t *label,const char *format,const char *value,lv_color_t value_color)
{
    const char *slot=format?strstr(format,"%s"):NULL;
    lv_obj_update_layout(lv_obj_get_parent(label));
    int width=lv_obj_get_content_width(lv_obj_get_parent(label));
    lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
    lv_obj_set_width(label,width);
    lv_obj_set_style_pad_all(label,0,0);
    lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
    if(!slot){
        while(lv_obj_get_child_count(label))lv_obj_delete(lv_obj_get_child(label,-1));
        lv_label_set_text(label,value?value:"");ui_text_guard(label);
        lv_obj_set_height(label,LV_SIZE_CONTENT);return;
    }
    if(*lv_label_get_text(label))lv_label_set_text(label,"");
    while(lv_obj_get_child_count(label)<3){
        lv_obj_t *child=lv_label_create(label);
        lv_obj_add_flag(child,LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_label_set_long_mode(child,LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(child,LV_TEXT_ALIGN_LEFT,0);
    }
    lv_obj_t *before=lv_obj_get_child(label,0),*variable=lv_obj_get_child(label,1),*after=lv_obj_get_child(label,2);
    lv_obj_set_style_text_color(variable,value_color,0);
    char prefix[160];size_t bytes=(size_t)(slot-format);
    if(bytes>=sizeof(prefix))bytes=sizeof(prefix)-1;
    memcpy(prefix,format,bytes);prefix[bytes]=0;
    const char *texts[]={prefix,value?value:"",slot+2};lv_obj_t *parts[]={before,variable,after};
    int measured[3],height=0;
    for(unsigned i=0;i<3;i++){
        if(strcmp(lv_label_get_text(parts[i]),texts[i]))lv_label_set_text(parts[i],texts[i]);
        ui_text_guard(parts[i]);lv_point_t size;
        const lv_font_t *font=lv_obj_get_style_text_font(parts[i],LV_PART_MAIN);
        lv_text_get_size(&size,texts[i],font,lv_obj_get_style_text_letter_space(parts[i],LV_PART_MAIN),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
        measured[i]=size.x+lv_obj_get_style_pad_left(parts[i],LV_PART_MAIN)+lv_obj_get_style_pad_right(parts[i],LV_PART_MAIN);
        int line=font->line_height+lv_obj_get_style_pad_top(parts[i],LV_PART_MAIN)+lv_obj_get_style_pad_bottom(parts[i],LV_PART_MAIN);
        lv_obj_set_height(parts[i],line);if(line>height)height=line;
    }
    int available=LV_MAX(1,width-measured[0]-measured[2]);
    bool overflow=measured[1]>available;
    int valueWidth=LV_MIN(measured[1],available),x=overflow?0:LV_MAX(0,(width-measured[0]-valueWidth-measured[2])/2);
    lv_label_long_mode_t mode=overflow?LV_LABEL_LONG_SCROLL:LV_LABEL_LONG_CLIP;
    if(lv_label_get_long_mode(variable)!=mode)lv_label_set_long_mode(variable,mode);
    lv_obj_set_height(label,height);
    int widths[]={measured[0],valueWidth,measured[2]};
    for(unsigned i=0;i<3;i++){
        lv_obj_set_size(parts[i],LV_MAX(1,widths[i]),height);
        lv_obj_set_pos(parts[i],x,0);x+=widths[i];
    }
}
