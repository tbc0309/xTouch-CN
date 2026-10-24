#pragma once
#include "features/ui_profile.h"
#include <lvgl.h>
#include "text_fit.h"
#if XTOUCH_UI_HIDPI_ASSETS
LV_FONT_DECLARE(ui_font_actions);
#endif
LV_FONT_DECLARE(ui_font_xlcd);
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
static inline void ui_layout_ink_place(lv_obj_t *label,int x,int y,int w,int h){
 ui_text_guard(label);
 lv_area_t b=ui_layout_ink_spaced(lv_obj_get_style_text_font(label,0),lv_label_get_text(label),lv_obj_get_style_text_letter_space(label,0));
 if(b.x2<b.x1)return;
 lv_obj_set_align(label,LV_ALIGN_TOP_LEFT);
 lv_obj_set_pos(label,x+(w-(b.x2-b.x1+1))/2-b.x1-lv_obj_get_style_pad_left(label,0),y+(h-(b.y2-b.y1+1))/2-b.y1-lv_obj_get_style_pad_top(label,0));
}
static inline void ui_layout_temperature_arrange(lv_obj_t *row,bool horizontal){
 int w=lv_obj_get_width(row),h=lv_obj_get_height(row);int scale=lv_disp_get_hor_res(NULL)>=600?2:1;lv_obj_t *icon=lv_obj_get_child(row,0),*value=lv_obj_get_child(row,1);
 lv_area_t ib=ui_layout_ink(lv_obj_get_style_text_font(icon,0),lv_label_get_text(icon)),vb=ui_layout_ink(lv_obj_get_style_text_font(value,0),lv_label_get_text(value));
 int ih=ib.y2-ib.y1+1,vh=vb.y2-vb.y1+1,vw=vb.x2-vb.x1+1;
 if(horizontal){ui_text_guard(icon);lv_obj_set_align(icon,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(icon,w/2-34*scale-ib.x1-lv_obj_get_style_pad_left(icon,0),(h-ih)/2-ib.y1-lv_obj_get_style_pad_top(icon,0));ui_layout_ink_place(value,w/2-7*scale,0,vw,h);}
 else{int y=(h-ih-3*scale-vh)/2;ui_layout_ink_place(icon,0,y,w,ih);ui_layout_ink_place(value,0,y+ih+3*scale,w,vh);}
}
static inline void ui_layout_temperature_layout_event(lv_event_t *e){
 lv_obj_t *o=lv_event_get_target(e);if(lv_obj_check_type(o,&lv_label_class))o=lv_obj_get_parent(o);
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
/* Keep icons and captions together. Both fonts shrink by the same ratio. */
static inline void ui_layout_action_fit(lv_obj_t *button,int width,int height,int pad,int gap){
 if(lv_obj_get_child_cnt(button)!=2)return;
 lv_obj_t *icon=lv_obj_get_child(button,0),*label=lv_obj_get_child(button,1);
 const lv_font_t *a=ui_text_source(lv_obj_get_style_text_font(icon,0)),*b=ui_text_source(lv_obj_get_style_text_font(label,0));
 lv_point_t aw,bw;lv_txt_get_size(&aw,lv_label_get_text(icon),a,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);lv_txt_get_size(&bw,lv_label_get_text(label),b,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 lv_area_t ai=ui_layout_ink(a,lv_label_get_text(icon)),bi=ui_layout_ink(b,lv_label_get_text(label));
 int reserve=(ai.x1<0?-ai.x1:0)+(ai.x2+1>aw.x?ai.x2+1-aw.x:0)+(bi.x1<0?-bi.x1:0)+(bi.x2+1>bw.x?bi.x2+1-bw.x:0);
 unsigned zoom=(width-2*pad-gap)*256/(aw.x+bw.x+reserve);if(zoom>ui_layout_base_zoom())zoom=ui_layout_base_zoom();
 int maxh=a->line_height>b->line_height?a->line_height:b->line_height;
 if((int)zoom*maxh>(height-2*pad)*256)zoom=(height-2*pad)*256/maxh;
 ui_text_scale(icon,a,zoom);ui_text_scale(label,b,zoom);lv_obj_clear_flag(icon,LV_OBJ_FLAG_HIDDEN);
 ui_text_guard(icon);ui_text_guard(label);lv_obj_set_size(icon,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_set_size(label,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_update_layout(icon);lv_obj_update_layout(label);
 lv_area_t ia=ui_layout_ink(lv_obj_get_style_text_font(icon,0),lv_label_get_text(icon)),ta=ui_layout_ink(lv_obj_get_style_text_font(label,0),lv_label_get_text(label));
 int iw=ia.x2-ia.x1+1,tw=ta.x2-ta.x1+1,start=(width-iw-gap-tw)/2;
 ui_layout_ink_place(icon,start,0,iw,height);ui_layout_ink_place(label,start+iw+gap,0,tw,height);
}
static inline void ui_layout_device_model(lv_obj_t *row){
 if(lv_obj_get_child_cnt(row)!=2)return;
 int scale=lv_disp_get_hor_res(NULL)>=600?2:1;
 ui_layout_action_fit(row,lv_obj_get_width(row),lv_obj_get_height(row),2*scale,3*scale);
}
static inline void ui_layout_device_model_event(lv_event_t *e){ui_layout_device_model(lv_event_get_target(e));}
/* Compact telemetry rows keep icon + value together and fit one native line. */
static inline void ui_layout_print_metric(lv_obj_t *row){
 if(lv_obj_get_child_cnt(row)!=2)return;
 int scale=lv_disp_get_hor_res(NULL)>=600?2:1;
 for(unsigned i=0;i<2;++i){lv_obj_t *child=lv_obj_get_child(row,i);lv_obj_add_flag(child,LV_OBJ_FLAG_IGNORE_LAYOUT);}
 lv_obj_t *icon=lv_obj_get_child(row,0),*label=lv_obj_get_child(row,1);
 const lv_font_t *a=ui_text_source(lv_obj_get_style_text_font(icon,0)),*b=ui_text_source(lv_obj_get_style_text_font(label,0));
 ui_text_scale(icon,a,ui_layout_base_zoom());ui_text_guard(icon);
 lv_area_t ib=ui_layout_ink(lv_obj_get_style_text_font(icon,0),lv_label_get_text(icon));int iw=ib.x2-ib.x1+1,h=lv_obj_get_height(row),w=lv_obj_get_width(row),gap=2*scale;
 int available=w-iw-gap; if(available<1)return;
 lv_area_t ink=ui_layout_ink(b,lv_label_get_text(label));int tw=ink.x2-ink.x1+1;unsigned zoom=tw>available?available*256/tw:256;if(zoom>ui_layout_base_zoom())zoom=ui_layout_base_zoom();
 ui_text_scale(label,b,zoom);ui_text_guard(label);
 lv_obj_set_size(icon,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_set_size(label,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_update_layout(label);
 ink=ui_layout_ink(lv_obj_get_style_text_font(label,0),lv_label_get_text(label));tw=ink.x2-ink.x1+1;
 ui_layout_ink_place(icon,0,0,iw,h);ui_layout_ink_place(label,iw+gap,0,available,h);
}
static inline void ui_layout_print_metric_event(lv_event_t *e){ui_layout_print_metric(lv_event_get_target(e));}
static inline void ui_layout_print_metrics(lv_obj_t *bar){
 if(lv_obj_get_child_cnt(bar)!=2)return;
 int minimum=0;
 for(unsigned r=0;r<2;++r){lv_obj_t *row=lv_obj_get_child(bar,r);
  for(unsigned i=0;i<lv_obj_get_child_cnt(row);++i){lv_obj_t *label=lv_obj_get_child(row,i);lv_area_t ink=ui_layout_ink(ui_text_source(lv_obj_get_style_text_font(label,0)),lv_label_get_text(label));int height=((ink.y2-ink.y1+1)*ui_layout_base_zoom()+255)/256;if(height>minimum)minimum=height;}
 }
 if(lv_obj_get_content_height(bar)<minimum){lv_obj_set_height(bar,minimum+2);lv_obj_update_layout(bar);}
 int gap=lv_disp_get_hor_res(NULL)>=600?6:3,w=lv_obj_get_content_width(bar),h=lv_obj_get_content_height(bar);
 if(w<=gap||h<=0)return;
 lv_obj_t *time=lv_obj_get_child(bar,0),*layers=lv_obj_get_child(bar,1);
 int left=(w-gap)/2;
 if(lv_obj_get_child_cnt(time)==2){
  lv_obj_t *icon=lv_obj_get_child(time,0),*label=lv_obj_get_child(time,1);
  lv_area_t a=ui_layout_ink(ui_text_source(lv_obj_get_style_text_font(icon,0)),lv_label_get_text(icon));
  lv_area_t b=ui_layout_ink(ui_text_source(lv_obj_get_style_text_font(label,0)),lv_label_get_text(label));
  int needed=((a.x2-a.x1+1+b.x2-b.x1+1)*ui_layout_base_zoom()+255)/256+(lv_disp_get_hor_res(NULL)>=600?4:2);
  int maximum=(w-gap)*2/3;
  if(needed>left)left=needed<maximum?needed:maximum;
 }
 lv_obj_set_pos(time,0,0);lv_obj_set_size(time,left,h);
 lv_obj_set_pos(layers,left+gap,0);lv_obj_set_size(layers,w-gap-left,h);
 lv_obj_update_layout(bar);
 ui_layout_print_metric(time);ui_layout_print_metric(layers);
}
static inline void ui_layout_print_metrics_event(lv_event_t *e){ui_layout_print_metrics(lv_event_get_target(e));}
/* Runs in native pixels on every board, including the P4 design-unit adapter. */
static inline void ui_layout_logo_accent(lv_obj_t *accent){
 lv_label_set_long_mode(accent,LV_LABEL_LONG_CLIP);
 lv_font_glyph_dsc_t glyph;if(lv_font_get_glyph_dsc(lv_obj_get_style_text_font(accent,0),&glyph,'4',0))(lv_obj_set_width)(accent,glyph.adv_w/6);
}
static inline void ui_layout_lamp_event(lv_event_t *e){
 lv_obj_t *card=lv_event_get_target(e),*icon=lv_obj_get_child(card,0);
 int w=lv_obj_get_content_width(card),h=lv_obj_get_content_height(card);
 if(w<1||h<1)return;
 {
  const lv_font_t *f=ui_text_source(lv_obj_get_style_text_font(icon,0));lv_area_t b=ui_layout_ink(f,lv_label_get_text(icon));
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
 const lv_font_t *font=ui_text_source(lv_obj_get_style_text_font(button,0));
#if XTOUCH_UI_HIDPI_ASSETS
 font=&ui_font_actions;
#endif
 lv_area_t base=ui_layout_ink(font,lv_label_get_text(button));
 int target=lv_obj_get_height(button)/2,available=lv_obj_get_width(button)/2;
 unsigned zoom=ui_layout_base_zoom();
 if(base.y2>=base.y1&&base.y2-base.y1+1>target)zoom=target*256/(base.y2-base.y1+1);
 if(base.x2>=base.x1&&base.x2-base.x1+1>available&&(unsigned)(available*256/(base.x2-base.x1+1))<zoom)zoom=available*256/(base.x2-base.x1+1);
 ui_text_scale(button,font,zoom);font=lv_obj_get_style_text_font(button,0);
 lv_area_t ink=ui_layout_ink(font,lv_label_get_text(button));
 int w=lv_obj_get_width(button),h=lv_obj_get_height(button);
 lv_obj_set_style_text_align(button,LV_TEXT_ALIGN_LEFT,0);
 lv_obj_set_style_pad_left(button,(w-(ink.x2-ink.x1+1))/2-ink.x1,0);
 lv_obj_set_style_pad_top(button,(h-(ink.y2-ink.y1+1))/2-ink.y1,0);
 lv_obj_set_style_pad_right(button,0,0);lv_obj_set_style_pad_bottom(button,0,0);
}
static inline void ui_layout_notice(lv_obj_t *box,lv_obj_t *label){
 int sw=lv_disp_get_hor_res(NULL),sh=lv_disp_get_ver_res(NULL),pad=sw/40,w=sw-2*pad;
 const lv_font_t *captionFont=lv_obj_get_style_text_font(label,0);
 lv_obj_remove_style_all(box);lv_obj_set_style_text_font(label,captionFont,0);lv_obj_set_style_bg_color(box,lv_color_hex(0x243A46),0);lv_obj_set_style_bg_opa(box,LV_OPA_COVER,0);lv_obj_set_style_radius(box,pad,0);lv_obj_set_style_pad_all(box,pad,0);
 lv_obj_set_width(label,w-2*pad);lv_obj_set_style_text_color(label,lv_color_white(),0);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);ui_text_guard(label);
 lv_obj_update_layout(label);int h=lv_obj_get_height(label);if(h>sh/2-2*pad){h=sh/2-2*pad;lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);lv_obj_set_height(label,h);}
 lv_obj_set_size(box,w,h+2*pad);lv_obj_align(box,LV_ALIGN_BOTTOM_MID,0,-pad);
 lv_obj_clear_flag(box,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(box,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_FLOATING);
}
static inline void ui_layout_dialog(lv_obj_t *panel,lv_obj_t *body,lv_obj_t **buttons,unsigned count,bool content){
 int sw=lv_disp_get_hor_res(NULL),sh=lv_disp_get_ver_res(NULL);int width=sw*(content?4:3)/5,height=sh*(content?8:6)/15,pad=sw/40,bh=sh*2/15,gap=sw*3/200;unsigned visible=0;
 lv_obj_set_layout(panel,0);lv_obj_set_style_pad_all(panel,0,0);lv_obj_set_style_border_width(panel,0,0);lv_obj_set_style_radius(panel,sw/50,0);
 for(unsigned i=0;i<count;i++)visible+=!lv_obj_has_flag(buttons[i],LV_OBJ_FLAG_HIDDEN);
 lv_obj_t *caption=lv_obj_get_child(body,0);lv_obj_set_align(caption,LV_ALIGN_TOP_LEFT);ui_text_guard(caption);lv_obj_set_width(caption,width-2*pad);lv_obj_update_layout(caption);
 int needed=lv_obj_get_height(caption)+2*pad+(visible?bh+gap:0);
 if(needed>height&&height==sh*2/5){width=sw*4/5;height=sh*8/15;ui_text_guard(caption);lv_obj_set_width(caption,width-2*pad);lv_obj_update_layout(caption);needed=lv_obj_get_height(caption)+2*pad+(visible?bh+gap:0);}
 if(needed>height)height=sh*13/15;
 lv_obj_set_size(panel,width,height);lv_obj_center(panel);
 lv_obj_set_pos(body,pad,pad);lv_obj_set_size(body,width-2*pad,height-2*pad-(visible?bh+gap:0));lv_obj_set_style_max_height(body,height,0);
 lv_obj_set_align(caption,lv_obj_get_height(caption)<=height-2*pad-(visible?bh+gap:0)?LV_ALIGN_CENTER:LV_ALIGN_TOP_LEFT);
 int slot=0,buttonWidth=visible?(width-2*pad-gap*(visible-1))/visible:0;
 for(unsigned i=0;i<count;i++){if(lv_obj_has_flag(buttons[i],LV_OBJ_FLAG_HIDDEN))continue;lv_obj_set_flex_grow(buttons[i],0);lv_obj_set_flex_align(buttons[i],LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_set_pos(buttons[i],pad+slot*(buttonWidth+gap),height-pad-bh);lv_obj_set_size(buttons[i],buttonWidth,bh);lv_obj_set_style_pad_all(buttons[i],0,0);lv_obj_set_style_border_width(buttons[i],0,0);lv_obj_set_style_pad_column(buttons[i],0,0);
 ui_layout_action_fit(buttons[i],buttonWidth,bh,sw/100,sw/100);
 slot++;}
}

static inline void ui_layout_numeric_center(lv_obj_t *ta){
 const lv_font_t *f=lv_obj_get_style_text_font(ta,0);const char *s=lv_textarea_get_text(ta);
 if(!*s)s=lv_textarea_get_placeholder_text(ta);
 if(!s||!*s)s="0";
 lv_area_t b=ui_layout_ink(f,s);int edge=lv_obj_get_style_border_width(ta,0);lv_point_t advance;
 lv_txt_get_size(&advance,s,f,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int shift=(advance.x-b.x1-b.x2-1)/2,extra=shift<0?-shift:shift;
 lv_obj_set_style_pad_left(ta,extra+shift,0);lv_obj_set_style_pad_right(ta,extra-shift,0);
 int top=(lv_obj_get_height(ta)-edge*2-(b.y2-b.y1+1))/2-b.y1;
 lv_obj_set_style_text_align(ta,LV_TEXT_ALIGN_CENTER,0);lv_obj_set_style_text_align(ta,LV_TEXT_ALIGN_CENTER,LV_PART_TEXTAREA_PLACEHOLDER);
 lv_obj_set_style_pad_top(ta,top>0?top:0,0);lv_obj_set_style_pad_bottom(ta,0,0);
}
static inline void ui_layout_numeric_center_event(lv_event_t *e){ui_layout_numeric_center(lv_event_get_target(e));}

/* Keys are clickable labels: center their ink inside the fixed background. */
static inline void ui_layout_key_event(lv_event_t *e){
 lv_obj_t *key=lv_event_get_target(e);const lv_font_t *f=lv_obj_get_style_text_font(key,0);const char *text=lv_label_get_text(key);
 lv_area_t b=ui_layout_ink(f,text);if(b.x2<b.x1)return;lv_point_t advance;lv_txt_get_size(&advance,text,f,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int shift=(advance.x-b.x1-b.x2-1)/2,extra=shift<0?-shift:shift;
 lv_obj_set_style_pad_left(key,extra+shift,0);lv_obj_set_style_pad_right(key,extra-shift,0);
 lv_obj_set_style_pad_top(key,(lv_obj_get_height(key)-(b.y2-b.y1+1))/2-b.y1,0);lv_obj_set_style_pad_bottom(key,0,0);
}
static inline void ui_layout_key(lv_obj_t *key){
 lv_obj_set_style_pad_all(key,0,0);lv_obj_set_style_text_align(key,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_event_cb(key,ui_layout_key_event,LV_EVENT_SIZE_CHANGED,NULL);
}

/* Knock out only the signal glyph bounds, keeping the rest of Wi-Fi visible. */
static inline void ui_layout_signal_background(lv_event_t *e){
 lv_obj_t *label=lv_event_get_target(e),*badge=lv_obj_get_parent(label);
 lv_area_t b=ui_layout_ink(lv_obj_get_style_text_font(label,0),lv_label_get_text(label)),a;
 if(b.x2<b.x1)return;
 lv_obj_get_coords(label,&a);
 int x=a.x1+lv_obj_get_style_pad_left(label,0),y=a.y1+lv_obj_get_style_pad_top(label,0);
 a.x1=x+b.x1-1;a.x2=x+b.x2+1;a.y1=y+b.y1-1;a.y2=y+b.y2+1;
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
 d.bg_color=lv_obj_get_style_bg_color(lv_obj_get_parent(lv_obj_get_parent(badge)),0);
 d.bg_opa=LV_OPA_COVER;lv_draw_rect(lv_event_get_draw_ctx(e),&d,&a);
}

static inline void ui_layout_status(lv_obj_t *badge){
 lv_obj_t *icon=lv_obj_get_child(badge,0),*signal=lv_obj_get_child(badge,1);
 const char *current=lv_label_get_text(signal);if(!current[0]||(current[0]=='-'&&!current[1]))lv_label_set_text(signal,"--");
 int size=lv_obj_get_style_text_font(icon,0)->line_height,h=lv_obj_get_height(badge);
 const lv_font_t *signalFont=ui_text_source(lv_obj_get_style_text_font(signal,0));
 ui_text_scale(signal,signalFont,signalFont->line_height<=20?220:135);
 lv_obj_set_style_text_color(signal,lv_color_hex(0xFFFFFF),0);
 ui_layout_ink_place(icon,0,0,size,h);
 lv_area_t ib=ui_layout_ink(lv_obj_get_style_text_font(icon,0),lv_label_get_text(icon));
 lv_area_t sb=ui_layout_ink(lv_obj_get_style_text_font(signal,0),lv_label_get_text(signal));
 lv_font_glyph_dsc_t minus,digit;const char *text=lv_label_get_text(signal);unsigned length=strlen(text);uint32_t first=length>1?(unsigned char)text[1]:'0',next=length>2?(unsigned char)text[2]:0;lv_font_get_glyph_dsc(lv_obj_get_style_text_font(signal,0),&minus,'-',first);lv_font_get_glyph_dsc(lv_obj_get_style_text_font(signal,0),&digit,first,next);
 int iw=ib.x2-ib.x1+1,sw=sb.x2-sb.x1+1,overlap=minus.adv_w-sb.x1+digit.ofs_x;
 if(overlap<0)overlap=0;
 if(overlap>minus.adv_w)overlap=minus.adv_w;
 ui_layout_ink_place(icon,0,0,iw,h);
 ui_layout_ink_place(signal,iw-overlap,0,sw,h/2);
 int width=iw-overlap+sw+1;if(lv_obj_get_width(badge)!=width)lv_obj_set_width(badge,width);
}
static inline void ui_layout_match_width(lv_obj_t *target,const lv_obj_t *reference){lv_obj_set_width(target,lv_obj_get_width(reference));}
static inline void ui_layout_single_line(lv_obj_t *label){
 const lv_font_t *f=ui_text_source(lv_obj_get_style_text_font(label,0));
 ui_text_scale(label,f,ui_layout_base_zoom());ui_text_guard(label);
 int width=lv_obj_get_content_width(label)-2;if(width<=0)return;
 unsigned lo=1,hi=ui_layout_base_zoom();
 // Glyph advances round to whole pixels. Fit the rendered font, not a ratio
 // of its original width, which leaves substantial unused space on small LCDs.
 while(lo<hi){unsigned mid=(lo+hi+1)/2;ui_text_scale(label,f,mid);lv_point_t extent;
  int spacing=lv_obj_get_style_text_letter_space(label,0);
  lv_txt_get_size(&extent,lv_label_get_text(label),lv_obj_get_style_text_font(label,0),spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  lv_area_t ink=ui_layout_ink_spaced(lv_obj_get_style_text_font(label,0),lv_label_get_text(label),spacing);
  if(extent.x<=width&&ink.x2<width)lo=mid;else hi=mid-1;
 }
 ui_text_scale(label,f,lo);ui_text_guard(label);
}
static inline void ui_layout_single_line_event(lv_event_t *e){ui_layout_single_line(lv_event_get_target(e));}
static inline void ui_layout_project_text(lv_obj_t *label,int width){
 const lv_font_t *source=ui_text_source(lv_obj_get_style_text_font(label,0));unsigned lo=1,hi=ui_layout_base_zoom();
 lv_obj_add_flag(label,UI_TEXT_EXACT_BOUNDS);
 int spacing=lv_obj_get_style_text_letter_space(label,0);
 while(lo<hi){unsigned mid=(lo+hi+1)/2;ui_text_scale(label,source,mid);const lv_font_t *f=lv_obj_get_style_text_font(label,0);lv_point_t extent;
  lv_txt_get_size(&extent,lv_label_get_text(label),f,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);lv_area_t ink=ui_layout_ink(f,lv_label_get_text(label));ink.x2+=((int)_lv_txt_get_encoded_length(lv_label_get_text(label))-1)*spacing;int left=ink.x1<0?-ink.x1:0,right=ink.x2+1>extent.x?ink.x2+1-extent.x:0;
  if(extent.x+left+right+2<=width)lo=mid;else hi=mid-1;
 }
 ui_text_scale(label,source,lo);const lv_font_t *f=lv_obj_get_style_text_font(label,0);lv_point_t extent;lv_txt_get_size(&extent,lv_label_get_text(label),f,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);lv_area_t ink=ui_layout_ink(f,lv_label_get_text(label));ink.x2+=((int)_lv_txt_get_encoded_length(lv_label_get_text(label))-1)*spacing;
 lv_obj_set_style_pad_left(label,1+(ink.x1<0?-ink.x1:0),0);lv_obj_set_style_pad_right(label,1+(ink.x2+1>extent.x?ink.x2+1-extent.x:0),0);
 int top=2+(ink.y1<0?-ink.y1:0),bottom=2+(ink.y2>=f->line_height?ink.y2+1-f->line_height:0);
 lv_obj_set_style_pad_top(label,top,0);lv_obj_set_style_pad_bottom(label,bottom,0);
 lv_obj_set_height(label,f->line_height+top+bottom);
}
static inline void ui_layout_project_event(lv_event_t *e){
 lv_obj_t *row=lv_event_get_target(e),*icon=lv_obj_get_child(row,0),*label=lv_obj_get_child(row,1);lv_img_header_t image;
 int h=lv_obj_get_content_height(row),w=lv_obj_get_content_width(row);if(h<1||w<1)return;
 if(lv_disp_get_hor_res(NULL)<=480)h=20;
 if(lv_img_decoder_get_info(lv_img_get_src(icon),&image)==LV_RES_OK){lv_img_set_size_mode(icon,LV_IMG_SIZE_MODE_REAL);lv_img_set_zoom(icon,h*256/image.h);lv_obj_refresh_self_size(icon);}
 int available=w-lv_obj_get_width(icon)-lv_obj_get_style_pad_column(row,0);if(available<1)return;
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

static inline void ui_layout_status_event(lv_event_t *e){ui_layout_status(lv_event_get_target(e));}
static inline lv_obj_t *ui_layout_status_create(lv_obj_t *connection,lv_obj_t *icon,lv_obj_t *signal){
 int size=lv_obj_get_style_text_font(icon,0)->line_height;lv_obj_t *badge=lv_obj_create(connection);lv_obj_remove_style_all(badge);
 lv_obj_set_size(badge,size*5/4,lv_pct(100));lv_obj_clear_flag(badge,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_parent(icon,badge);lv_obj_set_parent(signal,badge);
 lv_obj_add_event_cb(signal,ui_layout_signal_background,LV_EVENT_DRAW_MAIN_BEGIN,NULL);
 lv_obj_add_event_cb(badge,ui_layout_status_event,LV_EVENT_SIZE_CHANGED,NULL);return badge;
}

static inline void ui_layout_center_icon_event(lv_event_t *e){
 lv_obj_t *label=lv_event_get_target(e),*parent=lv_obj_get_parent(label);
 const lv_font_t *source=ui_text_source(lv_obj_get_style_text_font(label,0));
 lv_area_t original=ui_layout_ink(source,lv_label_get_text(label));
 int iw=original.x2-original.x1+1,ih=original.y2-original.y1+1;
 int aw=lv_obj_get_content_width(parent)-4,ah=lv_obj_get_content_height(parent)-4;
 unsigned zoom=ui_layout_base_zoom();
 if(iw>aw&&aw>0)zoom=aw*256/iw;
 if(ih>ah&&ah>0&&(unsigned)(ah*256/ih)<zoom)zoom=ah*256/ih;
 ui_text_scale(label,source,zoom);ui_text_guard(label);lv_obj_center(label);
 lv_area_t b=ui_layout_ink(lv_obj_get_style_text_font(label,0),lv_label_get_text(label));lv_point_t size;
 const lv_font_t *f=lv_obj_get_style_text_font(label,0);lv_txt_get_size(&size,lv_label_get_text(label),f,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int pl=lv_obj_get_style_pad_left(label,0),pr=lv_obj_get_style_pad_right(label,0),pt=lv_obj_get_style_pad_top(label,0),pb=lv_obj_get_style_pad_bottom(label,0);
 lv_obj_set_style_translate_x(label,(size.x+pr-pl-(b.x2-b.x1+1))/2-b.x1,0);
 lv_obj_set_style_translate_y(label,(f->line_height+pb-pt-(b.y2-b.y1+1))/2-b.y1,0);
}
static inline void ui_layout_center_icon(lv_obj_t *label,const lv_font_t *font,const char *symbol){
 lv_obj_set_style_text_font(label,font,0);lv_label_set_text(label,symbol);lv_obj_add_flag(label,LV_OBJ_FLAG_IGNORE_LAYOUT);
 lv_obj_add_event_cb(label,ui_layout_center_icon_event,LV_EVENT_SIZE_CHANGED,NULL);lv_obj_center(label);
}
static inline void ui_layout_close(lv_obj_t *button,lv_obj_t *label,const lv_font_t *font){
 int side=lv_disp_get_ver_res(NULL)/8;lv_obj_set_size(button,side,side);lv_obj_align(button,LV_ALIGN_TOP_RIGHT,0,0);lv_obj_set_style_pad_all(button,0,0);
 ui_layout_center_icon(label,font,"r");
}
static inline void ui_layout_square(lv_obj_t *o,int designHeight){
 int side=designHeight*lv_disp_get_ver_res(NULL)/240;lv_obj_set_size(o,side,side);
}
static inline lv_area_t ui_layout_native_area(lv_area_t a){
 int w=lv_disp_get_hor_res(NULL),h=lv_disp_get_ver_res(NULL);
 return (lv_area_t){(lv_coord_t)(a.x1*w/320),(lv_coord_t)(a.y1*h/240),(lv_coord_t)((a.x2+1)*w/320-1),(lv_coord_t)((a.y2+1)*h/240-1)};
}
static inline void ui_layout_draw_text(lv_draw_ctx_t *ctx,const lv_draw_label_dsc_t *style,lv_area_t design,const char *text,bool center){
 lv_area_t area=ui_layout_native_area(design),clip;if(!_lv_area_intersect(&clip,&area,ctx->clip_area))return;
 lv_area_t b=ui_layout_ink(style->font,text);lv_draw_label_dsc_t d=*style;d.align=LV_TEXT_ALIGN_LEFT;d.flag=LV_TEXT_FLAG_EXPAND;
 area.x1+=(center?(lv_area_get_width(&area)-(b.x2-b.x1+1))/2:0)-b.x1;
 area.y1+=(lv_area_get_height(&area)-(b.y2-b.y1+1))/2-b.y1;
 const lv_area_t *previous=ctx->clip_area;ctx->clip_area=&clip;lv_draw_label(ctx,&d,&area,text,NULL);ctx->clip_area=previous;
}

/* Runtime-sized notification graphics must precede legacy coordinate macros. */
#include "message_style.h"

/* Shared appearance for screen-side network entry and return buttons. */
static inline void ui_layout_setup_button(lv_obj_t *button,bool management){
 lv_obj_set_style_radius(button,8,0);
 lv_obj_set_style_bg_color(button,lv_color_hex(management?0x425A54:0x3E5059),0);
 lv_obj_set_style_bg_color(button,lv_color_hex(management?0x324640:0x30424B),LV_STATE_PRESSED);
 lv_obj_set_style_text_color(button,lv_color_hex(0xE6ECEF),0);
 lv_obj_set_style_border_width(button,0,0);
 lv_obj_set_style_shadow_width(button,0,0);
}
/* Icons share a fixed slot and align by their visible glyph bounds. */
static inline void ui_layout_setup_symbol_event(lv_event_t *event){
 lv_obj_t *slot=lv_event_get_target(event),*icon=lv_obj_get_child(slot,0);
 if(icon)ui_layout_ink_place(icon,0,0,lv_obj_get_width(slot),lv_obj_get_height(slot));
}
/* Plain, centered captions shared by all screen-side network entries. */
static inline void ui_layout_setup_caption(lv_obj_t *button,const char *text){
 const int pad=lv_disp_get_hor_res(NULL)*16/320;
 lv_obj_set_style_pad_left(button,pad,0);lv_obj_set_style_pad_right(button,pad,0);lv_obj_set_style_pad_ver(button,0,0);
 lv_obj_t *label=lv_label_create(button);lv_obj_set_width(label,lv_pct(100));
 lv_label_set_text(label,text);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
 lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
 lv_obj_add_event_cb(label,ui_layout_single_line_event,LV_EVENT_SIZE_CHANGED,NULL);
 lv_obj_update_layout(button);ui_layout_single_line(label);lv_obj_center(label);
}

/* QR captions use the actual display coordinates, independent of design scaling. */
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
  lv_obj_set_style_text_color(label,lv_color_hex(i?0xBFCBD2:0xE1E8EC),0);
  lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
  if(address){
   const lv_font_t *source=&ui_font_text_28;unsigned lo=1,hi=256;
   while(lo<hi){unsigned mid=(lo+hi+1)/2;ui_text_scale(label,source,mid);lv_point_t size;
    lv_txt_get_size(&size,primary,lv_obj_get_style_text_font(label,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
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
