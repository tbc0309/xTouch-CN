#pragma once
#include "dialog_text.h"
#include "features/ui_profile.h"
#include "palette.h"
#include "ui/lvgl_compat.h"
#include "text_fit.h"
#include "native_fonts.h"
#include "chrome_metrics.h"
#include "ui_specs.h"
/* Native drawing entry points, captured before legacy design-coordinate aliases. */
static inline void ui_layout_native_content(const lv_obj_t *object,lv_area_t *area){lv_obj_get_content_coords(object,area);}
static inline void ui_layout_native_bounds(const lv_obj_t *object,lv_area_t *area){lv_obj_get_coords(object,area);}
static inline void ui_layout_native_rect(lv_draw_ctx_t *layer,const lv_draw_rect_dsc_t *style,const lv_area_t *area){lv_draw_rect(layer,style,area);}
static inline void ui_layout_native_measure(lv_point_t *size,const char *text,const lv_font_t *font){lv_text_get_size(size,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);}
#if XTOUCH_UI_HIDPI_ASSETS
LV_FONT_DECLARE(ui_font_actions);
#endif
LV_FONT_DECLARE(ui_font_xlcd);
LV_FONT_DECLARE(ui_font_xlcdmin);
// Font resources have a reference canvas; intermediate displays must scale
// their glyphs as well as their geometry. All layout measurements use ink.
static inline unsigned ui_layout_base_zoom(void){
#if XTOUCH_UI_HIDPI_ASSETS
 unsigned x=lv_disp_get_hor_res(NULL)*256/800,y=lv_disp_get_ver_res(NULL)*256/480;
 unsigned z=x<y?x:y;return z<256?z:256;
#else
 return 256;
#endif
}
/* Direct canvas text must use the same pre-generated size as widget text.
 * A font already owned/scaled by a widget must not be scaled a second time. */
static inline const lv_font_t *ui_layout_canvas_font(const lv_font_t *font,const char *text){
 if(!font)font=LV_FONT_DEFAULT;
 unsigned zoom=ui_layout_base_zoom();
 if(zoom>=256||ui_text_source(font)!=font)return font;
 const lv_font_t *native=ui_native_font_select(font,zoom,text);
 return native?native:font;
}
/* Visible UTF-8 glyph bounds, independent of advance width and baseline. */
static inline lv_area_t ui_layout_ink_spaced(const lv_font_t *f,const char *s,int spacing){
 lv_area_t b={32767,32767,-32767,-32767};int at=0;uint32_t index=0;
 while(s[index]){uint32_t code=_lv_txt_encoded_next(s,&index),peek=index,next=s[index]?_lv_txt_encoded_next(s,&peek):0;lv_font_glyph_dsc_t g;if(!lv_font_get_glyph_dsc(f,&g,code,next))continue;
  if(g.box_w&&g.box_h){int x=at+g.ofs_x,y=f->line_height-f->base_line-g.box_h-g.ofs_y;
   if(x<b.x1)b.x1=x;
   if(y<b.y1)b.y1=y;
   if(x+g.box_w-1>b.x2)b.x2=x+g.box_w-1;
   if(y+g.box_h-1>b.y2)b.y2=y+g.box_h-1;}
  at+=g.adv_w+spacing;
 }return b;
}
static inline lv_area_t ui_layout_ink(const lv_font_t *f,const char *s){return ui_layout_ink_spaced(f,s,0);}
/* Flex centers line boxes. Correct their asymmetric font bearings in native
   pixels so text and pictograms share the same visible vertical center. */
static inline void ui_layout_status_text(lv_obj_t *label){
 const lv_font_t *font=lv_obj_get_style_text_font(label,(lv_part_t)(0));
 lv_area_t ink=ui_layout_ink(font,lv_label_get_text(label));
 if(ink.y2>=ink.y1)lv_obj_set_style_translate_y(label,(font->line_height-(ink.y2-ink.y1+1))/2-ink.y1,0);
}
static inline void ui_layout_status_text_event(lv_event_t *event){ui_layout_status_text(lv_event_get_target_obj(event));}
/* Fixed caption plus a value window, measured entirely in native pixels.
 * Never mix a measured glyph width with design coordinates or shrink the
 * scrolling window below one complete primary reading. */
static inline void ui_layout_caption_value(lv_obj_t *name,lv_obj_t *value,const char *minimum,int gap){
 lv_obj_t *row=lv_obj_get_parent(name);lv_obj_update_layout(row);
 ui_text_guard(name);ui_text_guard(value);
 const lv_font_t *nf=lv_obj_get_style_text_font(name,(lv_part_t)0),*vf=lv_obj_get_style_text_font(value,(lv_part_t)0);
 lv_point_t ns,vs,ms;
 lv_text_get_size(&ns,lv_label_get_text(name),nf,lv_obj_get_style_text_letter_space(name,(lv_part_t)0),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 lv_text_get_size(&vs,lv_label_get_text(value),vf,lv_obj_get_style_text_letter_space(value,(lv_part_t)0),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 lv_text_get_size(&ms,minimum,vf,lv_obj_get_style_text_letter_space(value,(lv_part_t)0),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int np=lv_obj_get_style_pad_left(name,(lv_part_t)0)+lv_obj_get_style_pad_right(name,(lv_part_t)0);
 int vp=lv_obj_get_style_pad_left(value,(lv_part_t)0)+lv_obj_get_style_pad_right(value,(lv_part_t)0);
 int width=lv_obj_get_content_width(row),height=lv_obj_get_content_height(row),nw=ns.x+np;
 int available=width-nw-gap;bool fits=*minimum&&available>=ms.x+vp;
 int vw=fits?LV_MIN(available,vs.x+vp):0;
 int left=LV_MAX(0,(width-nw-(fits?gap+vw:0))/2);
 if(lv_label_get_long_mode(name)!=LV_LABEL_LONG_CLIP)lv_label_set_long_mode(name,LV_LABEL_LONG_CLIP);
 lv_label_long_mode_t mode=fits&&vs.x+vp>available?LV_LABEL_LONG_SCROLL:LV_LABEL_LONG_CLIP;
 if(lv_label_get_long_mode(value)!=mode)lv_label_set_long_mode(value,mode);
 if(fits)lv_obj_clear_flag(value,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(value,LV_OBJ_FLAG_HIDDEN);
 lv_obj_set_size(name,nw,nf->line_height+lv_obj_get_style_pad_top(name,(lv_part_t)0)+lv_obj_get_style_pad_bottom(name,(lv_part_t)0));
 lv_obj_set_size(value,LV_MAX(1,vw),vf->line_height+lv_obj_get_style_pad_top(value,(lv_part_t)0)+lv_obj_get_style_pad_bottom(value,(lv_part_t)0));
 lv_obj_t *labels[]={name,value};int xs[]={left,left+nw+gap};
 for(unsigned i=0;i<2;i++){
  lv_obj_t *label=labels[i];lv_area_t ink=ui_layout_ink(lv_obj_get_style_text_font(label,(lv_part_t)0),lv_label_get_text(label));
  lv_obj_set_align(label,LV_ALIGN_TOP_LEFT);
  lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
  lv_obj_set_pos(label,xs[i],(height-(ink.y2-ink.y1+1))/2-ink.y1-lv_obj_get_style_pad_top(label,(lv_part_t)0));
 }
}
static inline void ui_layout_ink_place(lv_obj_t *label,int x,int y,int w,int h){
 ui_text_guard(label);
 lv_area_t b=ui_layout_ink_spaced(lv_obj_get_style_text_font(label,(lv_part_t)(0)),lv_label_get_text(label),lv_obj_get_style_text_letter_space(label,(lv_part_t)(0)));
 if(b.x2<b.x1)return;
 lv_obj_set_align(label,LV_ALIGN_TOP_LEFT);
 lv_obj_set_pos(label,x+(w-(b.x2-b.x1+1))/2-b.x1-lv_obj_get_style_pad_left(label,(lv_part_t)(0)),y+(h-(b.y2-b.y1+1))/2-b.y1-lv_obj_get_style_pad_top(label,(lv_part_t)(0)));
}
/* Normalize ordinary button captions after constructing a component/factory.
 * Dedicated icon/group layouts retain ownership of IGNORE_LAYOUT labels. */
#ifdef __cplusplus
extern "C" {
#endif
void ui_layout_captions_tree(lv_obj_t *root);
void ui_layout_setup_choice_content(lv_obj_t *card,const char *symbol,const char *title,const char *detail);
#ifdef __cplusplus
}
#endif
static inline void ui_layout_temperature_arrange(lv_obj_t *row,bool horizontal){
 int w=lv_obj_get_width(row),h=lv_obj_get_height(row);int scale=lv_disp_get_hor_res(NULL)>=600?2:1;lv_obj_t *icon=lv_obj_get_child(row,0),*value=lv_obj_get_child(row,1);
 lv_area_t ib=ui_layout_ink(lv_obj_get_style_text_font(icon,(lv_part_t)(0)),lv_label_get_text(icon)),vb=ui_layout_ink(lv_obj_get_style_text_font(value,(lv_part_t)(0)),lv_label_get_text(value));
 int ih=ib.y2-ib.y1+1,vh=vb.y2-vb.y1+1,vw=vb.x2-vb.x1+1;
 if(horizontal){ui_text_guard(icon);lv_obj_set_align(icon,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(icon,w/2-34*scale-ib.x1-lv_obj_get_style_pad_left(icon,(lv_part_t)(0)),(h-ih)/2-ib.y1-lv_obj_get_style_pad_top(icon,(lv_part_t)(0)));ui_layout_ink_place(value,w/2-7*scale,0,vw,h);}
 else{int y=(h-ih-3*scale-vh)/2;ui_layout_ink_place(icon,0,y,w,ih);ui_layout_ink_place(value,0,y+ih+3*scale,w,vh);}
}
static inline void ui_layout_temperature_layout_event(lv_event_t *e){
 lv_obj_t *o=lv_event_get_target_obj(e);if(lv_obj_check_type(o,&lv_label_class))o=lv_obj_get_parent(o);
 ui_layout_temperature_arrange(o,(uintptr_t)lv_event_get_user_data(e)!=0);
}
static inline void ui_layout_temperature_setup(lv_obj_t *row,bool horizontal){
 lv_obj_set_style_pad_all(row,0,0);
 for(int i=0;i<2;i++){lv_obj_t *l=lv_obj_get_child(row,i);lv_obj_add_flag(l,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_width(l,LV_SIZE_CONTENT);lv_obj_set_height(l,LV_SIZE_CONTENT);lv_obj_set_style_pad_all(l,0,0);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_LEFT,0);lv_obj_add_event_cb(l,ui_layout_temperature_layout_event,LV_EVENT_SIZE_CHANGED,(void *)(uintptr_t)horizontal);}
 lv_obj_add_event_cb(row,ui_layout_temperature_layout_event,LV_EVENT_SIZE_CHANGED,(void *)(uintptr_t)horizontal);
}
static inline void ui_layout_action(lv_obj_t *button,const lv_font_t *font,const char *symbol){
 lv_label_set_text(button,"");lv_obj_set_layout(button,0);
 lv_obj_t *icon=lv_label_create(button);lv_obj_set_style_text_font(icon,font,0);lv_label_set_text(icon,symbol);lv_obj_move_to_index(icon,0);
 for(unsigned i=0;i<2;i++)lv_obj_add_flag(lv_obj_get_child(button,i),LV_OBJ_FLAG_IGNORE_LAYOUT);
}
/* Icon/caption geometry is implemented once in ui_helpers.c. */
/* Control cards center the visible glyphs, not unequal icon/number line boxes. */
static inline void ui_layout_control_card(lv_obj_t *card){
 static bool busy=false;
 if(busy)return;
 int width=lv_obj_get_width(card),height=lv_obj_get_height(card);
 if(width<=0||height<=0)return;
 busy=true;
 if(lv_obj_get_child_cnt(card)==2){ui_specs_t m=ui_specs_get();ui_layout_action_fit(card,width,height,m.inset,m.gap);}
 else if(lv_obj_get_child_cnt(card)==1){
  lv_obj_t *icon=lv_obj_get_child(card,0);
  ui_text_scale(icon,ui_text_source(lv_obj_get_style_text_font(icon,(lv_part_t)(0))),ui_layout_base_zoom());
  ui_layout_ink_place(icon,0,0,width,height);
 }
 busy=false;
}
static inline void ui_layout_control_card_event(lv_event_t *event){
 lv_obj_t *target=lv_event_get_target_obj(event);
 ui_layout_control_card(lv_obj_check_type(target,&lv_label_class)?lv_obj_get_parent(target):target);
}
static inline void ui_layout_control_card_setup(lv_obj_t *card){
 lv_obj_set_layout(card,0);lv_obj_set_style_pad_all(card,0,0);
 for(unsigned i=0;i<lv_obj_get_child_cnt(card);++i){
  lv_obj_t *label=lv_obj_get_child(card,i);
  lv_obj_add_flag(label,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_flex_grow(label,0);
  lv_obj_set_size(label,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_set_style_pad_all(label,0,0);
  lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
  lv_obj_add_event_cb(label,ui_layout_control_card_event,LV_EVENT_SIZE_CHANGED,NULL);
 }
 lv_obj_add_event_cb(card,ui_layout_control_card_event,LV_EVENT_SIZE_CHANGED,NULL);
}
static inline void ui_layout_device_model(lv_obj_t *row){
 if(lv_obj_get_child_cnt(row)!=2)return;
 ui_specs_t m=ui_specs_get();
 ui_layout_action_fit(row,lv_obj_get_width(row),lv_obj_get_height(row),m.inset,m.gap);
}
static inline void ui_layout_device_model_event(lv_event_t *e){ui_layout_device_model(lv_event_get_target_obj(e));}
/* Compact telemetry rows keep icon + value together and fit one native line. */
static inline void ui_layout_print_metric(lv_obj_t *row){
 if(lv_obj_get_child_cnt(row)!=2)return;
 int scale=lv_disp_get_hor_res(NULL)>=600?2:1;
 for(unsigned i=0;i<2;++i){lv_obj_t *child=lv_obj_get_child(row,i);lv_obj_add_flag(child,LV_OBJ_FLAG_IGNORE_LAYOUT);}
 lv_obj_t *icon=lv_obj_get_child(row,0),*label=lv_obj_get_child(row,1);
 const lv_font_t *a=ui_text_source(lv_obj_get_style_text_font(icon,(lv_part_t)(0))),*b=ui_text_source(lv_obj_get_style_text_font(label,(lv_part_t)(0)));
 ui_text_scale(icon,a,ui_layout_base_zoom());ui_text_guard(icon);
 lv_area_t ib=ui_layout_ink(lv_obj_get_style_text_font(icon,(lv_part_t)(0)),lv_label_get_text(icon));int iw=ib.x2-ib.x1+1,h=lv_obj_get_height(row),w=lv_obj_get_width(row),gap=2*scale;
 int available=w-iw-gap; if(available<1)return;
 lv_area_t ink=ui_layout_ink(b,lv_label_get_text(label));int tw=ink.x2-ink.x1+1;unsigned zoom=tw>available?available*256/tw:256;if(zoom>ui_layout_base_zoom())zoom=ui_layout_base_zoom();
 ui_text_scale(label,b,zoom);ui_text_guard(label);
 // Scaled glyph bearings can round one pixel wider than their advance width.
 for(unsigned correction=0;correction<256 && zoom>1;++correction){
  lv_area_t actual=ui_layout_ink(lv_obj_get_style_text_font(label,LV_PART_MAIN),lv_label_get_text(label));
  if(actual.x2-actual.x1+1<=available)break;
  ui_text_scale(label,b,--zoom);
 }
 lv_obj_set_size(icon,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_set_size(label,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_update_layout(label);
 ink=ui_layout_ink(lv_obj_get_style_text_font(label,LV_PART_MAIN),lv_label_get_text(label));tw=ink.x2-ink.x1+1;
 ui_layout_ink_place(icon,0,0,iw,h);ui_layout_ink_place(label,iw+gap,0,available,h);
}
static inline void ui_layout_print_metric_event(lv_event_t *e){ui_layout_print_metric(lv_event_get_target_obj(e));}
static inline void ui_layout_print_metrics(lv_obj_t *bar){
 if(lv_obj_get_child_cnt(bar)!=2)return;
 int minimum=0;
 for(unsigned r=0;r<2;++r){lv_obj_t *row=lv_obj_get_child(bar,r);
  for(unsigned i=0;i<lv_obj_get_child_cnt(row);++i){lv_obj_t *label=lv_obj_get_child(row,i);lv_area_t ink=ui_layout_ink(ui_text_source(lv_obj_get_style_text_font(label,(lv_part_t)(0))),lv_label_get_text(label));int height=((ink.y2-ink.y1+1)*ui_layout_base_zoom()+255)/256;if(height>minimum)minimum=height;}
 }
 if(lv_obj_get_content_height(bar)<minimum){lv_obj_set_height(bar,minimum+2);lv_obj_update_layout(bar);}
 int gap=lv_disp_get_hor_res(NULL)>=600?6:3,w=lv_obj_get_content_width(bar),h=lv_obj_get_content_height(bar);
 if(w<=gap||h<=0)return;
 lv_obj_t *time=lv_obj_get_child(bar,0),*layers=lv_obj_get_child(bar,1);
 int left=(w-gap)/2;
 if(lv_obj_get_child_cnt(time)==2){
  lv_obj_t *icon=lv_obj_get_child(time,0),*label=lv_obj_get_child(time,1);
  lv_area_t a=ui_layout_ink(ui_text_source(lv_obj_get_style_text_font(icon,(lv_part_t)(0))),lv_label_get_text(icon));
  lv_area_t b=ui_layout_ink(ui_text_source(lv_obj_get_style_text_font(label,(lv_part_t)(0))),lv_label_get_text(label));
  int needed=((a.x2-a.x1+1+b.x2-b.x1+1)*ui_layout_base_zoom()+255)/256+(lv_disp_get_hor_res(NULL)>=600?4:2);
  int maximum=(w-gap)*2/3;
  if(needed>left)left=needed<maximum?needed:maximum;
 }
 lv_obj_set_pos(time,0,0);lv_obj_set_size(time,left,h);
 lv_obj_set_pos(layers,left+gap,0);lv_obj_set_size(layers,w-gap-left,h);
 lv_obj_update_layout(bar);
 ui_layout_print_metric(time);ui_layout_print_metric(layers);
}
static inline void ui_layout_print_metrics_event(lv_event_t *e){ui_layout_print_metrics(lv_event_get_target_obj(e));}
/* Runs in native pixels on every board, including the P4 design-unit adapter. */
static inline void ui_layout_logo_accent(lv_obj_t *accent){
 lv_label_set_long_mode(accent,LV_LABEL_LONG_CLIP);
 lv_font_glyph_dsc_t glyph;if(lv_font_get_glyph_dsc(lv_obj_get_style_text_font(accent,(lv_part_t)(0)),&glyph,'4',0))(lv_obj_set_width)(accent,glyph.adv_w/6);
}
static inline void ui_layout_lamp_event(lv_event_t *e){
 lv_obj_t *card=lv_event_get_target_obj(e),*icon=lv_obj_get_child(card,0);
 int w=lv_obj_get_content_width(card),h=lv_obj_get_content_height(card);
 if(w<1||h<1)return;
 {
  const lv_font_t *f=ui_text_source(lv_obj_get_style_text_font(icon,(lv_part_t)(0)));lv_area_t b=ui_layout_ink(f,lv_label_get_text(icon));
  int target=h/2,ih=b.y2-b.y1+1;if(ih>0)ui_text_scale(icon,f,target*256/ih);
 }
 ui_layout_ink_place(icon,0,0,w,h);
}
static inline void ui_layout_lamp(lv_obj_t *card,lv_obj_t *icon){
#if XTOUCH_UI_HIDPI_ASSETS
 lv_obj_set_style_text_font(icon,&ui_font_actions,0);
#else
 lv_obj_set_style_text_font(icon,&ui_font_xlcd,0);
#endif
 lv_obj_add_flag(icon,LV_OBJ_FLAG_IGNORE_LAYOUT);
 lv_obj_add_event_cb(card,ui_layout_lamp_event,LV_EVENT_SIZE_CHANGED,NULL);
 lv_obj_add_event_cb(card,ui_layout_lamp_event,LV_EVENT_STYLE_CHANGED,NULL);
}
static inline void ui_layout_print_button(lv_obj_t *button){
 const lv_font_t *font=ui_text_source(lv_obj_get_style_text_font(button,(lv_part_t)(0)));
#if XTOUCH_UI_HIDPI_ASSETS
 font=&ui_font_actions;
#endif
 lv_area_t base=ui_layout_ink(font,lv_label_get_text(button));
 int target=lv_obj_get_height(button)/2,available=lv_obj_get_width(button)/2;
 unsigned zoom=ui_layout_base_zoom();
 if(base.y2>=base.y1&&base.y2-base.y1+1>target)zoom=target*256/(base.y2-base.y1+1);
 if(base.x2>=base.x1&&base.x2-base.x1+1>available&&(unsigned)(available*256/(base.x2-base.x1+1))<zoom)zoom=available*256/(base.x2-base.x1+1);
 ui_text_scale(button,font,zoom);font=lv_obj_get_style_text_font(button,(lv_part_t)(0));
 lv_area_t ink=ui_layout_ink(font,lv_label_get_text(button));
 int w=lv_obj_get_width(button),h=lv_obj_get_height(button);
 lv_obj_set_style_text_align(button,LV_TEXT_ALIGN_LEFT,0);
 lv_obj_set_style_pad_left(button,(w-(ink.x2-ink.x1+1))/2-ink.x1,0);
 lv_obj_set_style_pad_top(button,(h-(ink.y2-ink.y1+1))/2-ink.y1,0);
 lv_obj_set_style_pad_right(button,0,0);lv_obj_set_style_pad_bottom(button,0,0);
}
static inline void ui_layout_notice(lv_obj_t *box,lv_obj_t *label){
 int sw=lv_disp_get_hor_res(NULL),sh=lv_disp_get_ver_res(NULL),pad=sw/40,w=sw-2*pad;
 const lv_font_t *captionFont=lv_obj_get_style_text_font(label,(lv_part_t)(0));
 lv_obj_remove_style_all(box);lv_obj_set_style_text_font(label,captionFont,0);lv_obj_set_style_bg_color(box,lv_color_hex(0x555555),0);lv_obj_set_style_bg_opa(box,LV_OPA_COVER,0);lv_obj_set_style_radius(box,pad,0);lv_obj_set_style_pad_all(box,pad,0);
 lv_obj_set_width(label,w-2*pad);lv_obj_set_style_text_color(label,lv_color_white(),0);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);ui_text_guard(label);
 ui_dialog_text_layout(label,w-2*pad);lv_obj_update_layout(label);int h=lv_obj_get_height(label);if(h>sh/2-2*pad)h=sh/2-2*pad;
 lv_obj_set_size(box,w,h+2*pad);lv_obj_align(box,LV_ALIGN_BOTTOM_MID,0,-pad);
 lv_obj_clear_flag(box,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(box,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_FLOATING);
}
static inline void ui_layout_dialog(lv_obj_t *panel,lv_obj_t *body,lv_obj_t **buttons,unsigned count,bool content){
 int sw=lv_disp_get_hor_res(NULL),sh=lv_disp_get_ver_res(NULL);int width=sw*(content?4:3)/5,pad=sw/40,bh=sh*2/15,gap=sw*3/200;unsigned visible=0;
 lv_obj_set_layout(panel,0);lv_obj_set_style_pad_all(panel,0,0);lv_obj_set_style_border_width(panel,0,0);lv_obj_set_style_radius(panel,sw/50,0);
 for(unsigned i=0;i<count;i++)visible+=!lv_obj_has_flag(buttons[i],LV_OBJ_FLAG_HIDDEN);
 int bodyInset=lv_obj_get_style_pad_left(body,(lv_part_t)(0))+lv_obj_get_style_pad_right(body,(lv_part_t)(0));
 lv_obj_t *caption=lv_obj_get_child(body,0);lv_obj_set_align(caption,LV_ALIGN_TOP_LEFT);ui_text_guard(caption);ui_dialog_text_layout(caption,width-2*pad-bodyInset);lv_obj_update_layout(caption);
 int verticalInset=lv_obj_get_style_pad_top(body,LV_PART_MAIN)+lv_obj_get_style_pad_bottom(body,LV_PART_MAIN);
 int needed=lv_obj_get_height(caption)+verticalInset+2*pad+(visible?bh+gap:0);
 /* Fit the current content; only cap at the available screen height. */
 int height=needed<sh*13/15?needed:sh*13/15;
 lv_obj_set_size(panel,width,height);lv_obj_center(panel);
 lv_obj_set_pos(body,pad,pad);lv_obj_set_size(body,width-2*pad,height-2*pad-(visible?bh+gap:0));lv_obj_set_style_max_height(body,height,0);
 bool overflow=needed>height;
 lv_obj_set_scroll_dir(body,LV_DIR_VER);
 lv_obj_set_scrollbar_mode(body,overflow?LV_SCROLLBAR_MODE_AUTO:LV_SCROLLBAR_MODE_OFF);
 if(overflow)lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);else lv_obj_clear_flag(body,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_align(caption,overflow?LV_ALIGN_TOP_LEFT:LV_ALIGN_CENTER);
 lv_obj_scroll_to_x(body,0,LV_ANIM_OFF);
 int slot=0,buttonWidth=visible?(width-2*pad-gap*(visible-1))/visible:0;
 for(unsigned i=0;i<count;i++){if(lv_obj_has_flag(buttons[i],LV_OBJ_FLAG_HIDDEN))continue;lv_obj_set_flex_grow(buttons[i],0);lv_obj_set_flex_align(buttons[i],LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_set_pos(buttons[i],pad+slot*(buttonWidth+gap),height-pad-bh);lv_obj_set_size(buttons[i],buttonWidth,bh);lv_obj_set_style_pad_all(buttons[i],0,0);lv_obj_set_style_border_width(buttons[i],0,0);lv_obj_set_style_pad_column(buttons[i],0,0);
 ui_layout_action_fit(buttons[i],buttonWidth,bh,sw/100,sw/100);
 slot++;}
}

static inline void ui_layout_numeric_center(lv_obj_t *ta){
 const lv_font_t *f=lv_obj_get_style_text_font(ta,(lv_part_t)(0));const char *s=lv_textarea_get_text(ta);
 if(!*s)s=lv_textarea_get_placeholder_text(ta);
 if(!s||!*s)s="0";
 lv_area_t b=ui_layout_ink(f,s);int edge=lv_obj_get_style_border_width(ta,(lv_part_t)(0));lv_point_t advance;
 lv_text_get_size(&advance,s,f,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int shift=(advance.x-b.x1-b.x2-1)/2,extra=shift<0?-shift:shift;
 lv_obj_set_style_pad_left(ta,extra+shift,0);lv_obj_set_style_pad_right(ta,extra-shift,0);
 int top=(lv_obj_get_height(ta)-edge*2-(b.y2-b.y1+1))/2-b.y1;
 lv_obj_set_style_text_align(ta,LV_TEXT_ALIGN_CENTER,0);lv_obj_set_style_text_align(ta,LV_TEXT_ALIGN_CENTER,LV_PART_TEXTAREA_PLACEHOLDER);
 lv_obj_set_style_pad_top(ta,top>0?top:0,0);lv_obj_set_style_pad_bottom(ta,0,0);
}
/* Numeric editor rows follow the actual shared keyboard grid, not fixed Y offsets. */
static inline void ui_layout_numeric_rows(lv_obj_t *left,lv_obj_t *input,lv_obj_t *more,lv_obj_t *seven,lv_obj_t *zero){
 lv_obj_update_layout(lv_scr_act());lv_area_t origin,row;lv_obj_get_content_coords(left,&origin);
 lv_obj_get_coords(seven,&row);lv_obj_set_y(input,row.y1-origin.y1);lv_obj_set_height(input,lv_area_get_height(&row));ui_layout_numeric_center(input);
 if(more){lv_obj_get_coords(zero,&row);lv_obj_set_y(more,row.y1-origin.y1);lv_obj_set_height(more,lv_area_get_height(&row));lv_obj_update_layout(more);ui_layout_ink_place(lv_obj_get_child(more,0),0,0,lv_obj_get_width(more),lv_obj_get_height(more));}
 lv_obj_update_layout(left);
}
static inline void ui_layout_numeric_center_event(lv_event_t *e){ui_layout_numeric_center(lv_event_get_target_obj(e));}

/* Clickable labels keep their background fixed and center the visible glyphs. */
static inline void ui_layout_button_text(lv_obj_t *key){
 const lv_font_t *f=lv_obj_get_style_text_font(key,(lv_part_t)(0));const char *text=lv_label_get_text(key);
 lv_area_t b=ui_layout_ink(f,text);if(b.x2<b.x1)return;lv_point_t advance;lv_text_get_size(&advance,text,f,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 lv_label_long_mode_t mode=advance.x+2>lv_obj_get_width(key)?LV_LABEL_LONG_SCROLL:LV_LABEL_LONG_CLIP;
 if(lv_label_get_long_mode(key)!=mode)lv_label_set_long_mode(key,mode);
 int shift=(advance.x-b.x1-b.x2-1)/2,extra=shift<0?-shift:shift;
 lv_obj_set_style_pad_left(key,extra+shift,0);lv_obj_set_style_pad_right(key,extra-shift,0);
 lv_obj_set_style_pad_top(key,(lv_obj_get_height(key)-(b.y2-b.y1+1))/2-b.y1,0);lv_obj_set_style_pad_bottom(key,0,0);
}
static inline void ui_layout_key_event(lv_event_t *e){ui_layout_button_text(lv_event_get_target_obj(e));}
static inline void ui_layout_key(lv_obj_t *key){
 lv_obj_set_style_pad_all(key,0,0);lv_obj_set_style_text_align(key,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_event_cb(key,ui_layout_key_event,LV_EVENT_SIZE_CHANGED,NULL);
}

/* Mask only the text/upper-right-quadrant intersection; preserve other arcs. */
static inline void ui_layout_signal_background(lv_event_t *e){
 lv_obj_t *label=lv_event_get_target_obj(e),*badge=lv_obj_get_parent(label),*icon=lv_obj_get_child(badge,0);
 lv_area_t ink=ui_layout_ink(lv_obj_get_style_text_font(icon,(lv_part_t)(0)),lv_label_get_text(icon));
 lv_area_t text=ui_layout_ink(lv_obj_get_style_text_font(label,(lv_part_t)(0)),lv_label_get_text(label)),a,q;
 int iw=ink.x2-ink.x1+1,ih=ink.y2-ink.y1+1;if(iw<1||ih<1)return;
 lv_obj_get_coords(badge,&q);q.y1+=(lv_obj_get_height(badge)-ih)/2;q.y2=q.y1+(ih+1)/2-1;
 q.x2=q.x1+iw-1;q.x1+=iw/2;
 lv_obj_get_coords(label,&a);int x=a.x1+lv_obj_get_style_pad_left(label,(lv_part_t)(0)),y=a.y1+lv_obj_get_style_pad_top(label,(lv_part_t)(0));
 a.x1=x+text.x1-1;a.x2=x+text.x2+1;a.y1=y+text.y1-1;a.y2=y+text.y2+1;
 if(a.x1<q.x1)a.x1=q.x1;if(a.x2>q.x2)a.x2=q.x2;
 if(a.y1<q.y1)a.y1=q.y1;if(a.y2>q.y2)a.y2=q.y2;
 if(a.x1>a.x2||a.y1>a.y2)return;
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
 d.bg_color=lv_obj_get_style_bg_color(lv_obj_get_parent(lv_obj_get_parent(badge)),(lv_part_t)(0));
 d.bg_opa=LV_OPA_COVER;lv_draw_rect(lv_event_get_draw_ctx(e),&d,&a);
}

/* Shared layout, with the native small/large icon resource selected by profile. */
static inline void ui_layout_status_icon(lv_obj_t *icon,int height){
 lv_area_t original=ui_layout_ink(&ui_font_xlcdmin,lv_label_get_text(icon));
 int inkHeight=original.y2-original.y1+1;
 if(height<1||inkHeight<1)return;
#if XTOUCH_UI_HIDPI_ASSETS
 /* Same MDI master as other large icons; 36 px equivalent, up from 32. */
 ui_text_scale(icon,&ui_font_xlcd,ui_layout_base_zoom()*3/4);
#else
 ui_text_scale(icon,&ui_font_xlcdmin,ui_layout_base_zoom());
#endif
 lv_obj_set_style_pad_all(icon,0,0);
 const lv_font_t *font=lv_obj_get_style_text_font(icon,(lv_part_t)(0));
 lv_area_t ink=ui_layout_ink(font,lv_label_get_text(icon));
 /* Flex rounds its line-box center; place actual ink at the row center. */
 lv_obj_set_style_translate_y(icon,(height-(ink.y2-ink.y1+1))/2-(height-font->line_height)/2-ink.y1,0);
}
/* Paint the independent click affordance after layout, using the final font.
   It stays visible while only the camera glyph's text opacity blinks. */
static inline void ui_layout_camera_access_dot(lv_event_t *e)
{
    lv_obj_t *icon=lv_event_get_target_obj(e);
    if(!lv_obj_has_flag(icon,LV_OBJ_FLAG_CLICKABLE))return;
    lv_area_t ink=ui_layout_ink(lv_obj_get_style_text_font(icon,(lv_part_t)(0)),lv_label_get_text(icon)),a;
    lv_obj_get_coords(icon,&a);
    int diameter=(ink.y2-ink.y1+1+4)/5;if(diameter<3)diameter=3;
    /* Anchor to the icon widget box, clear of the camera lens strokes. */
    a.x1=a.x2-diameter+1;a.y2=a.y1+diameter-1;
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
    d.bg_color=lv_color_hex(UI_COLOR_BLUE);d.bg_opa=LV_OPA_COVER;d.radius=LV_RADIUS_CIRCLE;
    lv_draw_rect(lv_event_get_draw_ctx(e),&d,&a);
}
/* Home fills from bottom; horizontal material bars fill from right. */
static inline void ui_layout_material_fill(lv_draw_ctx_t *ctx,const lv_area_t *area,const lv_draw_rect_dsc_t *style,lv_color_t color,unsigned percent,bool known,bool horizontal){
 lv_draw_rect_dsc_t d=*style;d.bg_color=color;
 if(!known||percent>=100){lv_draw_rect(ctx,&d,area);return;}
 d.bg_color=lv_color_mix(lv_color_white(),color,96);lv_draw_rect(ctx,&d,area);
 if(!percent)return;
 lv_area_t fill=*area,clip;
 if(horizontal)fill.x1=fill.x2-(lv_area_get_width(area)*percent+50)/100+1;
 else fill.y1=fill.y2-(lv_area_get_height(area)*percent+50)/100+1;
 if(fill.x1>fill.x2||fill.y1>fill.y2||!_lv_area_intersect(&clip,&fill,&ctx->_clip_area))return;
 lv_area_t old=ctx->_clip_area;ctx->_clip_area=clip;d.bg_color=color;lv_draw_rect(ctx,&d,area);ctx->_clip_area=old;
}
static inline void ui_layout_material_object(lv_event_t *e,lv_color_t color,unsigned percent,bool known){
 lv_obj_t *obj=lv_event_get_target_obj(e);lv_area_t a;lv_obj_get_coords(obj,&a);
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);lv_obj_init_draw_rect_dsc(obj,LV_PART_MAIN,&d);
 ui_layout_material_fill(lv_event_get_draw_ctx(e),&a,&d,color,percent,known,false);
}
static inline void ui_layout_camera_event(lv_event_t *e){
 lv_obj_t *icon=(lv_obj_t *)lv_event_get_user_data(e);
 ui_layout_status_icon(icon,lv_obj_get_content_height(lv_event_get_target_obj(e)));
}
/* Draw both selected text and symbol by their visible ink, rather than
   LVGL's shared top padding (the two fonts have different bearings). */
static inline lv_area_t ui_layout_dropdown_text_area(lv_obj_t *obj){
 lv_area_t a;lv_obj_get_coords(obj,&a);int border=lv_obj_get_style_border_width(obj,LV_PART_MAIN);
 a.x1+=border+lv_obj_get_style_pad_left(obj,LV_PART_MAIN);
 a.x2-=border+lv_obj_get_style_pad_right(obj,LV_PART_MAIN);
 const char *symbol=lv_dropdown_get_symbol(obj);
 if(symbol&&*symbol){
  const lv_font_t *font=lv_obj_get_style_text_font(obj,LV_PART_MAIN);int reserve=font->line_height;
  if(lv_img_src_get_type(symbol)==LV_IMG_SRC_SYMBOL){
   if(strcmp(symbol,LV_SYMBOL_DOWN)){lv_point_t size;lv_text_get_size(&size,symbol,lv_obj_get_style_text_font(obj,LV_PART_INDICATOR),0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);reserve=size.x;}
  }else{lv_img_header_t image;if(lv_img_decoder_get_info(symbol,&image)==LV_RES_OK)reserve=image.w;}
  reserve+=LV_MAX(3,font->line_height/4);
  bool rtl=lv_dropdown_get_dir(obj)==LV_DIR_LEFT||lv_obj_get_style_base_dir(obj,LV_PART_MAIN)==LV_BASE_DIR_RTL;
  if(rtl)a.x1+=reserve;else a.x2-=reserve;
 }
 return a;
}
/* Clip only the visible caption, keeping the full option in the dropdown list.
 * UTF-8 boundaries must survive truncation; never draw into the arrow slot. */
static inline const char *ui_layout_fit_caption(const lv_font_t *font,const char *text,int spacing,int width,char *buffer,size_t capacity){
 lv_point_t size;lv_text_get_size(&size,text,font,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 if(size.x<=width)return text;
 if(capacity<4)return "";
 lv_text_get_size(&size,"...",font,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 if(width<size.x)return "";
 size_t best=0;uint32_t index=0;buffer[0]=0;
 while(text[index]){
  _lv_txt_encoded_next(text,&index);if(index+4>capacity)break;
  memcpy(buffer,text,index);memcpy(buffer+index,"...",4);
  lv_text_get_size(&size,buffer,font,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  if(size.x>width)break;
  best=index;
 }
 memcpy(buffer,text,best);memcpy(buffer+best,"...",4);return buffer;
}
static inline void ui_layout_dropdown_draw(lv_event_t *e){
 lv_obj_t *obj=lv_event_get_target_obj(e);char selected[128];
 lv_dropdown_get_selected_str(obj,selected,sizeof(selected));
 const char *text=lv_dropdown_get_text(obj);if(!text)text=selected;
 const char *symbol=lv_dropdown_get_symbol(obj);
 lv_area_t a;lv_obj_get_coords(obj,&a);
 for(int part=0;part<2;++part){
  const char *value=part?symbol:text;if(!value||!*value)continue;
  if(part&&lv_img_src_get_type(value)!=LV_IMG_SRC_SYMBOL)continue;
  lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);
  /* Native text is suppressed with text_opa=0. LVGL then returns early
     before copying color/font into a descriptor, so read the real styles. */
  uint32_t selector=part?LV_PART_INDICATOR:LV_PART_MAIN;
  d.color=lv_obj_get_style_text_color_filtered(obj,(lv_part_t)(selector));
  d.font=lv_obj_get_style_text_font(obj,(lv_part_t)(selector));
  d.letter_space=lv_obj_get_style_text_letter_space(obj,(lv_part_t)(selector));
  d.line_space=lv_obj_get_style_text_line_space(obj,(lv_part_t)(selector));
  d.decor=lv_obj_get_style_text_decor(obj,(lv_part_t)(selector));
  d.align=lv_obj_get_style_text_align(obj,(lv_part_t)(selector));
  d.opa=part?lv_obj_get_style_opa(obj,(lv_part_t)(selector)):LV_OPA_COVER;
  if(part&&!strcmp(value,LV_SYMBOL_DOWN)){
   /* The compact body subset intentionally omits most FontAwesome glyphs.
    * Draw this common chevron in native pixels instead of relying on a
    * missing or oversized fallback glyph at intermediate resolutions. */
   int size=LV_MAX(6,lv_obj_get_style_text_font(obj,LV_PART_MAIN)->line_height*3/5);
   bool rtl=lv_dropdown_get_dir(obj)==LV_DIR_LEFT||lv_obj_get_style_base_dir(obj,LV_PART_MAIN)==LV_BASE_DIR_RTL;
   int border=lv_obj_get_style_border_width(obj,LV_PART_MAIN);
   int right=rtl?a.x1+border+lv_obj_get_style_pad_left(obj,LV_PART_MAIN)+size:a.x2-border-lv_obj_get_style_pad_right(obj,LV_PART_MAIN);
   int center=a.y1+lv_obj_get_height(obj)/2;
   lv_point_t p1={right-size,center-size/4},p2={right-size/2,center+size/4},p3={right,center-size/4};
   lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.color=d.color;line.opa=d.opa;
   line.width=LV_MAX(1,size/6);line.round_start=line.round_end=1;
   ui_draw_line(lv_event_get_draw_ctx(e),&line,&p1,&p2);ui_draw_line(lv_event_get_draw_ctx(e),&line,&p2,&p3);continue;
  }
#if LV_DRAW_COMPLEX
  if(part)d.blend_mode=lv_obj_get_style_blend_mode(obj,(lv_part_t)(selector));
#endif
#if LV_USE_BIDI
  d.bidi_dir=lv_obj_get_style_base_dir(obj,(lv_part_t)(LV_PART_MAIN));
#endif
  lv_area_t text_area=ui_layout_dropdown_text_area(obj);char fitted[192];
  if(!part)value=ui_layout_fit_caption(d.font,value,d.letter_space,lv_area_get_width(&text_area),fitted,sizeof(fitted));
  if(!*value)continue;
  lv_area_t ink=ui_layout_ink_spaced(d.font,value,d.letter_space);
  if(ink.y2<ink.y1)continue;
  lv_point_t size;lv_text_get_size(&size,value,d.font,d.letter_space,d.line_space,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  int border=lv_obj_get_style_border_width(obj,(lv_part_t)(0));
  int left=a.x1+border+lv_obj_get_style_pad_left(obj,(lv_part_t)(0)),right=a.x2-border-lv_obj_get_style_pad_right(obj,(lv_part_t)(0));
  bool rtl=lv_dropdown_get_dir(obj)==LV_DIR_LEFT||lv_obj_get_style_base_dir(obj,(lv_part_t)(0))==LV_BASE_DIR_RTL;
  lv_area_t draw;
  draw.x1=part?(rtl?left:right-size.x+1):(symbol?(rtl?text_area.x2-size.x+1:text_area.x1):text_area.x1+(lv_area_get_width(&text_area)-size.x)/2);
  draw.x2=draw.x1+size.x-1;
  draw.y1=a.y1+(lv_obj_get_height(obj)-(ink.y2-ink.y1+1))/2-ink.y1;
  draw.y2=draw.y1+size.y-1;
  lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(e);lv_area_t previous=ctx->_clip_area,clip;
  if(_lv_area_intersect(&clip,part?&a:&text_area,&previous)){
   ctx->_clip_area=clip;d.flag=LV_TEXT_FLAG_EXPAND;ui_draw_label(ctx,&d,&draw,value,NULL);ctx->_clip_area=previous;
  }
 }
}
static inline void ui_layout_dropdown(lv_obj_t *obj){
 lv_obj_set_style_text_opa(obj,LV_OPA_TRANSP,LV_PART_MAIN);
 lv_obj_set_style_text_opa(obj,LV_OPA_TRANSP,LV_PART_INDICATOR);
 lv_obj_set_style_text_opa(lv_dropdown_get_list(obj),LV_OPA_COVER,LV_PART_MAIN);
 lv_obj_remove_event_cb(obj,ui_layout_dropdown_draw);
 lv_obj_add_event_cb(obj,ui_layout_dropdown_draw,LV_EVENT_DRAW_MAIN_END,NULL);
}
static inline void ui_layout_status(lv_obj_t *badge){
 lv_obj_t *icon=lv_obj_get_child(badge,0),*signal=lv_obj_get_child(badge,1);
 int h=lv_obj_get_content_height(badge);if(h<1)return;
 const char *current=lv_label_get_text(signal);
 bool valid=current[0] && strcmp(current,"--") && strcmp(current,"-");
 if(valid)lv_obj_clear_flag(signal,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(signal,LV_OBJ_FLAG_HIDDEN);
 ui_layout_status_icon(icon,h);
 lv_obj_set_style_translate_y(icon,0,0);
 lv_area_t ib=ui_layout_ink(lv_obj_get_style_text_font(icon,(lv_part_t)(0)),lv_label_get_text(icon));
 int iw=ib.x2-ib.x1+1,ih=ib.y2-ib.y1+1;
 const lv_font_t *source=ui_text_source(lv_obj_get_style_text_font(signal,(lv_part_t)(0)));
#if XTOUCH_UI_HIDPI_ASSETS
 /* Large digits follow the visible Wi-Fi ink. */
 lv_area_t digit=ui_layout_ink(source,"0");
 int digitHeight=digit.y2-digit.y1+1,target=(ih*62+50)/100;
 ui_text_scale(signal,source,digitHeight>0?(target*256+digitHeight/2)/digitHeight:256);
#else
 /* Preserve the original small-screen RSSI size. */
 ui_text_scale(signal,source,source->line_height<=20?220:135);
#endif
 /* Reserve the same three-digit slot for every reading, including unknown.
    Proportional OPPO Sans glyphs must not move the following camera/clock. */
 const lv_font_t *sf=lv_obj_get_style_text_font(signal,(lv_part_t)(0));
 int slot=0;
 for(char d='0';d<='9';++d){char digits[4]={d,d,d,0};lv_area_t b=ui_layout_ink(sf,digits);int w=b.x2-b.x1+1;if(w>slot)slot=w;}
 int width=iw/2+slot;if(width<iw)width=iw;
 if(lv_obj_get_width(badge)!=width)lv_obj_set_width(badge,width);
 if(!valid){ui_layout_ink_place(icon,0,0,iw,h);return;}
 lv_area_t sb=ui_layout_ink(lv_obj_get_style_text_font(signal,(lv_part_t)(0)),lv_label_get_text(signal));
 int sw=sb.x2-sb.x1+1,sh=sb.y2-sb.y1+1,left=iw/2;
 int top=(h-ih)/2+(ih+1)/2-sh;if(top<0)top=0;
 lv_obj_set_style_text_color(signal,lv_color_hex(0xFFFFFF),0);
 ui_layout_ink_place(icon,0,0,iw,h);
 ui_layout_ink_place(signal,left,top,sw,sh);

}
static inline void ui_layout_match_width(lv_obj_t *target,const lv_obj_t *reference){lv_obj_set_width(target,lv_obj_get_width(reference));}
static inline void ui_layout_single_line(lv_obj_t *label){
 ui_text_scale(label,ui_text_source(lv_obj_get_style_text_font(label,LV_PART_MAIN)),ui_layout_base_zoom());
 ui_specs_text_window(label,lv_obj_get_width(label));
}
static inline void ui_layout_single_line_event(lv_event_t *e){ui_layout_single_line(lv_event_get_target_obj(e));}
static inline void ui_layout_project_text(lv_obj_t *label,int width){
 const lv_font_t *source=ui_text_source(lv_obj_get_style_text_font(label,(lv_part_t)(0)));unsigned lo=1,hi=ui_layout_base_zoom();
 lv_obj_add_flag(label,UI_TEXT_EXACT_BOUNDS);
 int spacing=lv_obj_get_style_text_letter_space(label,(lv_part_t)(0));
 while(lo<hi){unsigned mid=(lo+hi+1)/2;ui_text_scale(label,source,mid);const lv_font_t *f=lv_obj_get_style_text_font(label,(lv_part_t)(0));lv_point_t extent;
  lv_text_get_size(&extent,lv_label_get_text(label),f,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);lv_area_t ink=ui_layout_ink(f,lv_label_get_text(label));ink.x2+=((int)_lv_txt_get_encoded_length(lv_label_get_text(label))-1)*spacing;int left=ink.x1<0?-ink.x1:0,right=ink.x2+1>extent.x?ink.x2+1-extent.x:0;
  if(extent.x+left+right+2<=width)lo=mid;else hi=mid-1;
 }
 ui_text_scale(label,source,lo);const lv_font_t *f=lv_obj_get_style_text_font(label,(lv_part_t)(0));lv_point_t extent;lv_text_get_size(&extent,lv_label_get_text(label),f,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);lv_area_t ink=ui_layout_ink(f,lv_label_get_text(label));ink.x2+=((int)_lv_txt_get_encoded_length(lv_label_get_text(label))-1)*spacing;
 lv_obj_set_style_pad_left(label,1+(ink.x1<0?-ink.x1:0),0);lv_obj_set_style_pad_right(label,1+(ink.x2+1>extent.x?ink.x2+1-extent.x:0),0);
 int top=2+(ink.y1<0?-ink.y1:0),bottom=2+(ink.y2>=f->line_height?ink.y2+1-f->line_height:0);
 lv_obj_set_style_pad_top(label,top,0);lv_obj_set_style_pad_bottom(label,bottom,0);
 lv_obj_set_height(label,f->line_height+top+bottom);
}
static inline void ui_layout_project_event(lv_event_t *e){
 lv_obj_t *row=lv_event_get_target_obj(e),*icon=lv_obj_get_child(row,0),*label=lv_obj_get_child(row,1);lv_img_header_t image;
 int h=lv_obj_get_content_height(row),w=lv_obj_get_content_width(row);if(h<1||w<1)return;
 if(lv_disp_get_hor_res(NULL)<=480)h=20;
 if(lv_img_decoder_get_info(lv_img_get_src(icon),&image)==LV_RES_OK){lv_image_set_inner_align(icon,LV_IMAGE_ALIGN_CONTAIN);lv_obj_set_size(icon,h*image.w/image.h,h);}
 int available=w-lv_obj_get_width(icon)-lv_obj_get_style_pad_column(row,(lv_part_t)(0));if(available<1)return;
 lv_obj_set_width(label,available);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
 ui_layout_project_text(label,available);
}

static inline void ui_layout_qr_title(lv_obj_t *title,lv_obj_t *code){
 lv_obj_update_layout(title);ui_layout_single_line(title);
 /* Placement uses native top-left coordinates, regardless of the caller's anchor. */
 lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_LEFT,0);lv_obj_set_size(title,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
 lv_obj_update_layout(title);lv_obj_update_layout(code);
 ui_layout_ink_place(title,0,0,lv_obj_get_content_width(lv_obj_get_parent(title)),lv_obj_get_y(code));
}

static inline void ui_layout_status_event(lv_event_t *e){ui_layout_status(lv_event_get_target_obj(e));}
static inline lv_obj_t *ui_layout_status_create(lv_obj_t *connection,lv_obj_t *icon,lv_obj_t *signal){
 int size=lv_obj_get_style_text_font(icon,(lv_part_t)(0))->line_height;lv_obj_t *badge=lv_obj_create(connection);lv_obj_remove_style_all(badge);
 lv_obj_set_size(badge,size*5/4,lv_pct(100));lv_obj_clear_flag(badge,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_parent(icon,badge);lv_obj_set_parent(signal,badge);
 lv_obj_add_event_cb(signal,ui_layout_signal_background,LV_EVENT_DRAW_MAIN_BEGIN,NULL);
 lv_obj_add_event_cb(badge,ui_layout_status_event,LV_EVENT_SIZE_CHANGED,NULL);return badge;
}

static inline void ui_layout_center_icon_event(lv_event_t *e){
 lv_obj_t *label=lv_event_get_target_obj(e),*parent=lv_obj_get_parent(label);
 const lv_font_t *source=ui_text_source(lv_obj_get_style_text_font(label,(lv_part_t)(0)));
 lv_area_t original=ui_layout_ink(source,lv_label_get_text(label));
 int iw=original.x2-original.x1+1,ih=original.y2-original.y1+1;
 int aw=lv_obj_get_content_width(parent)-4,ah=lv_obj_get_content_height(parent)-4;
 unsigned zoom=ui_layout_base_zoom();
 if(iw>aw&&aw>0)zoom=aw*256/iw;
 if(ih>ah&&ah>0&&(unsigned)(ah*256/ih)<zoom)zoom=ah*256/ih;
 ui_text_scale(label,source,zoom);ui_text_guard(label);lv_obj_center(label);
 lv_area_t b=ui_layout_ink(lv_obj_get_style_text_font(label,(lv_part_t)(0)),lv_label_get_text(label));lv_point_t size;
 const lv_font_t *f=lv_obj_get_style_text_font(label,(lv_part_t)(0));lv_text_get_size(&size,lv_label_get_text(label),f,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int pl=lv_obj_get_style_pad_left(label,(lv_part_t)(0)),pr=lv_obj_get_style_pad_right(label,(lv_part_t)(0)),pt=lv_obj_get_style_pad_top(label,(lv_part_t)(0)),pb=lv_obj_get_style_pad_bottom(label,(lv_part_t)(0));
 lv_obj_set_style_translate_x(label,(size.x+pr-pl-(b.x2-b.x1+1))/2-b.x1,0);
 lv_obj_set_style_translate_y(label,(f->line_height+pb-pt-(b.y2-b.y1+1))/2-b.y1,0);
}
static inline void ui_layout_center_icon(lv_obj_t *label,const lv_font_t *font,const char *symbol){
 lv_obj_set_style_text_font(label,font,0);lv_label_set_text(label,symbol);lv_obj_add_flag(label,LV_OBJ_FLAG_IGNORE_LAYOUT);
 lv_obj_add_event_cb(label,ui_layout_center_icon_event,LV_EVENT_SIZE_CHANGED,NULL);lv_obj_center(label);
}
static inline void ui_layout_close(lv_obj_t *button,lv_obj_t *label,const lv_font_t *font){
 int side=ui_chrome_metrics(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL)).control;lv_obj_set_size(button,side,side);lv_obj_align(button,LV_ALIGN_TOP_RIGHT,0,0);lv_obj_set_style_pad_all(button,0,0);
 ui_layout_center_icon(label,font,"r");
}
static inline void ui_layout_square(lv_obj_t *o,int designHeight){
 int side=designHeight*lv_disp_get_ver_res(NULL)/240;lv_obj_set_size(o,side,side);
}
static inline lv_area_t ui_layout_native_area(lv_area_t a){
 int w=lv_disp_get_hor_res(NULL),h=lv_disp_get_ver_res(NULL);
 return (lv_area_t){(lv_coord_t)(a.x1*w/320),(lv_coord_t)(a.y1*h/240),(lv_coord_t)((a.x2+1)*w/320-1),(lv_coord_t)((a.y2+1)*h/240-1)};
}
static inline void ui_layout_draw_native_text(lv_draw_ctx_t *ctx,const lv_draw_label_dsc_t *style,lv_area_t area,const char *text,bool center){
 lv_area_t clip;if(!_lv_area_intersect(&clip,&area,&ctx->_clip_area))return;
 lv_area_t b=ui_layout_ink(style->font,text);lv_draw_label_dsc_t d=*style;d.align=LV_TEXT_ALIGN_LEFT;d.flag=LV_TEXT_FLAG_EXPAND;
 area.x1+=(center?(lv_area_get_width(&area)-(b.x2-b.x1+1))/2:0)-b.x1;
 area.y1+=(lv_area_get_height(&area)-(b.y2-b.y1+1))/2-b.y1;
 lv_area_t previous=ctx->_clip_area;ctx->_clip_area=clip;ui_draw_label(ctx,&d,&area,text,NULL);ctx->_clip_area=previous;
}
static inline void ui_layout_draw_text(lv_draw_ctx_t *ctx,const lv_draw_label_dsc_t *style,lv_area_t design,const char *text,bool center){
 lv_draw_label_dsc_t d=*style;d.font=ui_layout_canvas_font(d.font,text);
 ui_layout_draw_native_text(ctx,&d,ui_layout_native_area(design),text,center);
}

/* Runtime-sized notification graphics must precede legacy coordinate macros. */
#include "message_style.h"

/* Shared appearance for screen-side network entry and return buttons. */
static inline void ui_layout_setup_button(lv_obj_t *button,bool management){
 lv_obj_set_style_radius(button,8,0);
 lv_obj_set_style_bg_color(button,lv_color_hex(management?0x555555:0x555555),0);
 lv_obj_set_style_bg_color(button,lv_color_hex(management?0x777777:0x777777),LV_STATE_PRESSED);
 lv_obj_set_style_text_color(button,lv_color_hex(0xFFFFFF),0);
 lv_obj_set_style_border_width(button,0,0);
 lv_obj_set_style_shadow_width(button,0,0);
}
/* Icons share a fixed slot and align by their visible glyph bounds. */
static inline void ui_layout_setup_symbol_event(lv_event_t *event){
 lv_obj_t *slot=lv_event_get_target_obj(event),*icon=lv_obj_get_child(slot,0);
 if(icon)ui_layout_ink_place(icon,0,0,lv_obj_get_width(slot),lv_obj_get_height(slot));
}
/* Plain, centered captions shared by all screen-side network entries. */
static inline void ui_layout_setup_caption(lv_obj_t *button,const char *text){
 const int pad=lv_disp_get_hor_res(NULL)*16/320;
 lv_obj_set_style_pad_left(button,pad,0);lv_obj_set_style_pad_right(button,pad,0);lv_obj_set_style_pad_ver(button,0,0);
 lv_obj_t *label=lv_label_create(button);lv_obj_set_width(label,lv_pct(100));
 lv_label_set_text(label,text);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
 lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
 /* This factory owns width/scrolling. Do not also shrink a caption against
  * its own measured width, which can repeatedly reduce the font on resize. */
 ui_text_scale(label,ui_text_source(lv_obj_get_style_text_font(label,LV_PART_MAIN)),ui_layout_base_zoom());
 lv_obj_update_layout(button);ui_layout_captions_tree(button);
}

/* QR captions use the actual display coordinates, independent of design scaling. */
/* QR geometry is square and computed once in native pixels. */
static inline void ui_layout_qr_size(lv_obj_t *code){
 lv_obj_t *root=lv_obj_get_parent(code);lv_obj_update_layout(root);
 int height=lv_obj_get_content_height(root),width=lv_obj_get_content_width(root);
 int side=LV_MIN(height*128/240,width-32);
 lv_qrcode_set_size(code,side);lv_obj_align(code,LV_ALIGN_TOP_MID,0,height*38/240);
}
static inline lv_obj_t *ui_layout_qr_details(lv_obj_t *code,lv_obj_t *back,const char *primary,const char *secondary,bool address){
 lv_obj_t *parent=lv_obj_get_parent(code);lv_obj_update_layout(parent);
 const int width=address?lv_obj_get_width(code):lv_obj_get_width(parent)-16;
 lv_obj_t *box=lv_obj_create(parent);lv_obj_remove_style_all(box);
 lv_obj_clear_flag(box,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
 lv_obj_set_width(box,width);
 lv_obj_set_height(box,LV_SIZE_CONTENT);lv_obj_set_flex_flow(box,LV_FLEX_FLOW_COLUMN);
 lv_obj_set_flex_align(box,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
 lv_obj_set_style_pad_row(box,secondary?2:0,0);
 for(unsigned i=0;i<(secondary?2u:1u);++i){
  lv_obj_t *label=lv_label_create(box);lv_label_set_text(label,i?secondary:primary);
  lv_obj_set_width(label,LV_PCT(100));lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
  lv_obj_set_style_text_color(label,lv_color_hex(i?0xCCCCCC:0xFFFFFF),0);
  lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
  if(address){
   const lv_font_t *source=&ui_font_text_28;unsigned lo=1,hi=256;
   while(lo<hi){unsigned mid=(lo+hi+1)/2;ui_text_scale(label,source,mid);lv_point_t size;
    lv_text_get_size(&size,primary,lv_obj_get_style_text_font(label,(lv_part_t)(0)),0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
    if(size.x<=width-2)lo=mid;else hi=mid-1;
   }
   ui_text_scale(label,source,lo);
  }else ui_text_scale(label,LV_FONT_DEFAULT,224);
  ui_text_guard(label);
 }
 lv_obj_update_layout(parent);lv_area_t qr,limit,origin;lv_obj_get_coords(code,&qr);lv_obj_get_coords(back?back:parent,&limit);lv_obj_get_coords(parent,&origin);
 int bottom=back?limit.y1:limit.y2+1;
 lv_obj_set_pos(box,(lv_obj_get_width(parent)-lv_obj_get_width(box))/2,qr.y2+1-origin.y1+(bottom-qr.y2-1-lv_obj_get_height(box))/2);
 return box;
}
