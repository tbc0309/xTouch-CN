// Shared 320x240 design units -> current native display. Application files only.
#pragma once
#include "features/ui_profile.h"
#include "ui/lvgl_compat.h"
#include "layout_shared.h"
#include "message_style.h"
#if defined(XTOUCH_UI_NATIVE_CANVAS) && XTOUCH_UI_NATIVE_CANVAS
#ifdef __cplusplus
extern "C" {
#endif
// Percent/content/radius sentinels must retain LVGL's encoding.
static inline lv_coord_t ui_design_x(lv_coord_t v){return LV_COORD_IS_SPEC(v)||v==LV_COORD_MAX||v==LV_COORD_MIN||v==LV_RADIUS_CIRCLE?v:(lv_coord_t)((int64_t)v*lv_disp_get_hor_res(NULL)/320);}
static inline lv_coord_t ui_design_y(lv_coord_t v){return LV_COORD_IS_SPEC(v)||v==LV_COORD_MAX||v==LV_COORD_MIN||v==LV_RADIUS_CIRCLE?v:(lv_coord_t)((int64_t)v*lv_disp_get_ver_res(NULL)/240);}
static inline lv_obj_t *ui_design_create(lv_obj_t *parent){
 lv_obj_t *o=lv_obj_create(parent);
 if((!parent||parent==lv_layer_top()||parent==lv_layer_sys())&&ui_layout_base_zoom()<256)ui_text_scale(o,LV_FONT_DEFAULT,ui_layout_base_zoom());
 return o;
}
static inline void ui_design_font(lv_obj_t *o,const lv_font_t *font,lv_style_selector_t selector){
 if(selector==0&&ui_layout_base_zoom()<256)ui_text_scale(o,font,ui_layout_base_zoom());
 else lv_obj_set_style_text_font(o,font,selector);
}
static inline lv_area_t ui_design_area(const lv_area_t *a){return (lv_area_t){ui_design_x(a->x1),ui_design_y(a->y1),(lv_coord_t)(ui_design_x(a->x2+1)-1),(lv_coord_t)(ui_design_y(a->y2+1)-1)};}
static inline lv_coord_t ui_design_unx(lv_coord_t v){int w=lv_disp_get_hor_res(NULL);return v>=0?(v*320+w-1)/w:-((-v*320+w-1)/w);}
static inline lv_coord_t ui_design_uny(lv_coord_t v){int h=lv_disp_get_ver_res(NULL);return v>=0?(v*240+h-1)/h:-((-v*240+h-1)/h);}
static inline void ui_design_unarea(lv_area_t *a){a->x1=ui_design_unx(a->x1);a->x2=ui_design_unx(a->x2+1)-1;a->y1=ui_design_uny(a->y1);a->y2=ui_design_uny(a->y2+1)-1;}
static inline void ui_design_pos(lv_obj_t *o,lv_coord_t x,lv_coord_t y){lv_obj_set_pos(o,ui_design_x(x),ui_design_y(y));}
static inline void ui_design_size(lv_obj_t *o,lv_coord_t w,lv_coord_t h){lv_obj_set_size(o,ui_design_x(w),ui_design_y(h));}
static inline void ui_design_align(lv_obj_t *o,lv_align_t a,lv_coord_t x,lv_coord_t y){lv_obj_align(o,a,ui_design_x(x),ui_design_y(y));}
static inline void ui_design_coords(const lv_obj_t *o,lv_area_t *a){lv_obj_get_coords(o,a);ui_design_unarea(a);}
static inline void ui_design_content(const lv_obj_t *o,lv_area_t *a){lv_obj_get_content_coords(o,a);ui_design_unarea(a);}
// Exact inverse of floor(2.5*x): include the first native pixel of odd design columns.
static inline lv_coord_t ui_design_input_x(lv_coord_t native){return ((native+1)*320-1)/lv_disp_get_hor_res(NULL);}
static inline void ui_design_point(const lv_indev_t *i,lv_point_t *p){lv_indev_get_point(i,p);p->x=ui_design_input_x(p->x);p->y=((p->y+1)*240-1)/lv_disp_get_ver_res(NULL);}
static inline void ui_design_txt_size(lv_point_t *s,const char *t,const lv_font_t *f,lv_coord_t ls,lv_coord_t line,lv_coord_t w,lv_text_flag_t flag){lv_text_get_size(s,t,ui_layout_canvas_font(f,t),ui_design_x(ls),ui_design_y(line),ui_design_x(w),flag);s->x=(s->x*320+lv_disp_get_hor_res(NULL)-1)/lv_disp_get_hor_res(NULL);s->y=(s->y*240+lv_disp_get_ver_res(NULL)-1)/lv_disp_get_ver_res(NULL);}
static inline void ui_design_rect(lv_draw_ctx_t *c,const lv_draw_rect_dsc_t *d,const lv_area_t *a){lv_area_t n=ui_design_area(a);if(d->radius==LV_RADIUS_CIRCLE&&lv_area_get_width(a)==lv_area_get_height(a)){int trim=(lv_area_get_width(&n)-lv_area_get_height(&n))/2;n.x1+=trim;n.x2=n.x1+lv_area_get_height(&n)-1;}lv_draw_rect_dsc_t style=*d;style.radius=ui_design_y(style.radius);style.border_width=style.border_width*lv_disp_get_ver_res(NULL)/240;lv_draw_rect(c,&style,&n);}
static inline void ui_design_label(lv_draw_ctx_t *c,const lv_draw_label_dsc_t *d,const lv_area_t *a,const char *t,lv_draw_label_hint_t *h){lv_area_t n=ui_design_area(a),clip;if(!_lv_area_intersect(&clip,&n,&c->_clip_area))return;lv_draw_label_dsc_t style=*d;style.font=ui_layout_canvas_font(d->font,t);lv_area_t old=c->_clip_area;c->_clip_area=clip;ui_draw_label(c,&style,&n,t,h);c->_clip_area=old;}
static inline void ui_design_image(lv_draw_ctx_t *c,const lv_draw_img_dsc_t *d,const lv_area_t *a,const void *src){lv_area_t n=ui_design_area(a);lv_draw_img_dsc_t style=*d;lv_img_header_t h;if(lv_img_decoder_get_info(src,&h)!=LV_RES_OK)return;style.scale_x=style.scale_y=h.w==48?256:512;n.x2=n.x1+h.w-1;n.y2=n.y1+h.h-1;ui_draw_image(c,&style,&n,src);}

static inline const lv_area_t *ui_design_clip(lv_draw_ctx_t *c){static lv_area_t a;a=*&c->_clip_area;ui_design_unarea(&a);return &a;}
static inline lv_obj_t *ui_design_qr(lv_obj_t *o,lv_coord_t size,lv_color_t dark,lv_color_t light){return ui_qrcode_create(o,ui_design_y(size),dark,light);}
static inline void ui_design_set_width(lv_obj_t *o,lv_coord_t v){lv_obj_set_width(o,ui_design_x(v));}
static inline void ui_design_set_height(lv_obj_t *o,lv_coord_t v){lv_obj_set_height(o,ui_design_y(v));}
static inline void ui_design_set_x(lv_obj_t *o,lv_coord_t v){lv_obj_set_x(o,ui_design_x(v));}
static inline void ui_design_set_y(lv_obj_t *o,lv_coord_t v){lv_obj_set_y(o,ui_design_y(v));}
static inline void ui_design_style_width(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_width(o,ui_design_x(v),s);}
static inline void ui_design_style_height(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_height(o,ui_design_y(v),s);}
static inline void ui_design_style_max_height(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_max_height(o,ui_design_y(v),s);}
static inline void ui_design_style_pad_all(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_all(o,ui_design_y(v),s);}
static inline void ui_design_style_pad_ver(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_ver(o,ui_design_y(v),s);}
static inline void ui_design_style_pad_left(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_left(o,ui_design_x(v),s);}
static inline void ui_design_style_pad_right(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_right(o,ui_design_x(v),s);}
static inline void ui_design_style_pad_top(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_top(o,ui_design_y(v),s);}
static inline void ui_design_style_pad_bottom(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_bottom(o,ui_design_y(v),s);}
static inline void ui_design_style_pad_row(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_row(o,ui_design_y(v),s);}
static inline void ui_design_style_pad_column(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_pad_column(o,ui_design_x(v),s);}
static inline void ui_design_style_radius(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_radius(o,ui_design_y(v),s);}
static inline void ui_design_style_border_width(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_border_width(o,ui_design_y(v),s);}
static inline void ui_design_style_text_line_space(lv_obj_t *o,lv_coord_t v,lv_style_selector_t s){lv_obj_set_style_text_line_space(o,ui_design_y(v),s);}
static inline lv_coord_t ui_design_get_width(const lv_obj_t *o){return ui_design_unx(lv_obj_get_width(o));}
static inline lv_coord_t ui_design_get_height(const lv_obj_t *o){return ui_design_uny(lv_obj_get_height(o));}
static inline lv_coord_t ui_design_get_x_aligned(const lv_obj_t *o){return ui_design_unx(lv_obj_get_x_aligned(o));}
static inline lv_coord_t ui_design_get_y_aligned(const lv_obj_t *o){return ui_design_uny(lv_obj_get_y_aligned(o));}
static inline void ui_design_scroll_x(lv_obj_t *o,lv_coord_t v,lv_anim_enable_t a){lv_obj_scroll_to_x(o,ui_design_x(v),a);}
static inline void ui_design_scroll_y(lv_obj_t *o,lv_coord_t v,lv_anim_enable_t a){lv_obj_scroll_to_y(o,ui_design_y(v),a);}
#ifdef __cplusplus
}
#endif
#define lv_obj_set_pos ui_design_pos
#define lv_obj_set_size ui_design_size
#define lv_obj_align ui_design_align
#define lv_obj_get_coords ui_design_coords
#define lv_obj_get_content_coords ui_design_content
#define lv_indev_get_point ui_design_point
#define lv_text_get_size ui_design_txt_size
#define lv_draw_rect ui_design_rect
#define lv_draw_label ui_design_label
#undef ui_draw_design_label
#define ui_draw_design_label ui_design_label
#define lv_draw_img ui_design_image
#define lv_qrcode_create ui_design_qr
#define UI_DESIGN_CLIP_AREA ui_design_clip
#define lv_obj_set_width ui_design_set_width
#define lv_obj_set_height ui_design_set_height
#define lv_obj_set_x ui_design_set_x
#define lv_obj_set_y ui_design_set_y
#define lv_obj_set_style_width ui_design_style_width
#define lv_obj_set_style_height ui_design_style_height
#define lv_obj_set_style_max_height ui_design_style_max_height
#define lv_obj_set_style_pad_all ui_design_style_pad_all
#define lv_obj_set_style_pad_ver ui_design_style_pad_ver
#define lv_obj_set_style_pad_left ui_design_style_pad_left
#define lv_obj_set_style_pad_right ui_design_style_pad_right
#define lv_obj_set_style_pad_top ui_design_style_pad_top
#define lv_obj_set_style_pad_bottom ui_design_style_pad_bottom
#define lv_obj_set_style_pad_row ui_design_style_pad_row
#define lv_obj_set_style_pad_column ui_design_style_pad_column
#define lv_obj_set_style_radius ui_design_style_radius
#define lv_obj_set_style_border_width ui_design_style_border_width
#define lv_obj_set_style_text_line_space ui_design_style_text_line_space
#define lv_obj_create ui_design_create
#define lv_obj_set_style_text_font ui_design_font
#define lv_obj_get_width ui_design_get_width
#define lv_obj_get_height ui_design_get_height
#define lv_obj_get_x_aligned ui_design_get_x_aligned
#define lv_obj_get_y_aligned ui_design_get_y_aligned
#define lv_obj_scroll_to_x ui_design_scroll_x
#define lv_obj_scroll_to_y ui_design_scroll_y
#endif
