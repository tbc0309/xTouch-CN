#pragma once
#include <lvgl.h>
#include <initializer_list>
#include "ui/layout_shared.h"
LV_FONT_DECLARE(ui_font_camera_14);
LV_FONT_DECLARE(ui_font_xlcdmin);
LV_FONT_DECLARE(ui_font_text_24);
enum class CameraAction { Close, Light, Freeze, Retry };
struct CameraUi {
 lv_obj_t *overlay=nullptr,*image=nullptr,*caption=nullptr,*top=nullptr,*tools=nullptr,*buttons[4]={};
 void (*action)(CameraAction)=nullptr;
 uint32_t touched=0;bool visible=true,live=false,paused=false;
};
inline void cameraUiShow(CameraUi &u,bool show,uint32_t now){
 u.touched=now;u.visible=show;
 for(auto *o:{u.top,u.tools})if(show)lv_obj_clear_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
}
inline void cameraUiTap(lv_event_t *e){auto &u=*(CameraUi*)lv_event_get_user_data(e);cameraUiShow(u,!u.visible||!u.live||u.paused,lv_tick_get());}
inline void cameraUiButton(lv_event_t *e){
 auto &u=*(CameraUi*)lv_event_get_user_data(e);cameraUiShow(u,true,lv_tick_get());
 for(unsigned i=0;i<4;i++)if(lv_event_get_target(e)==u.buttons[i]&&u.action){u.action(static_cast<CameraAction>(i));break;}
}
/* Native pixels from the display, with no per-board layout branches. */
inline void cameraUiCreate(CameraUi &u,const char *message,void (*action)(CameraAction)){
 u={};u.action=action;u.touched=lv_tick_get();
 int w=lv_disp_get_hor_res(nullptr),h=lv_disp_get_ver_res(nullptr),edge=w/40,side=h>=400?64:48,bar=side+edge*2;
 u.overlay=lv_obj_create(lv_layer_top());lv_obj_remove_style_all(u.overlay);lv_obj_set_size(u.overlay,w,h);lv_obj_center(u.overlay);
 lv_obj_set_style_bg_color(u.overlay,lv_color_black(),0);lv_obj_set_style_bg_opa(u.overlay,LV_OPA_COVER,0);
 lv_obj_add_flag(u.overlay,LV_OBJ_FLAG_CLICKABLE);lv_obj_clear_flag(u.overlay,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_add_event_cb(u.overlay,cameraUiTap,LV_EVENT_CLICKED,&u);
 u.image=lv_img_create(u.overlay);lv_obj_center(u.image);lv_obj_clear_flag(u.image,LV_OBJ_FLAG_CLICKABLE);
 for(auto **p:{&u.top,&u.tools}){
  *p=lv_obj_create(u.overlay);lv_obj_remove_style_all(*p);lv_obj_set_size(*p,w,bar);
  lv_obj_set_style_bg_color(*p,lv_color_hex(0x151B22),0);lv_obj_set_style_bg_opa(*p,LV_OPA_80,0);
  lv_obj_clear_flag(*p,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
 }
 lv_obj_align(u.top,LV_ALIGN_TOP_MID,0,0);lv_obj_align(u.tools,LV_ALIGN_BOTTOM_MID,0,0);
 u.caption=lv_label_create(u.top);lv_obj_set_style_text_font(u.caption,&ui_font_camera_14,0);
 lv_obj_set_style_text_color(u.caption,lv_color_white(),0);lv_obj_set_width(u.caption,w-side-3*edge);
 lv_label_set_long_mode(u.caption,LV_LABEL_LONG_WRAP);lv_label_set_text(u.caption,message);ui_text_guard(u.caption);lv_obj_align(u.caption,LV_ALIGN_LEFT_MID,edge,0);
 const char *symbols[]={"r","w",LV_SYMBOL_PAUSE,LV_SYMBOL_REFRESH};int bw=(w-4*edge)/3;
 for(unsigned i=0;i<4;i++){
  auto *button=u.buttons[i]=lv_btn_create(i?u.tools:u.top);lv_obj_remove_style_all(button);lv_obj_set_style_pad_all(button,0,0);
  lv_obj_set_size(button,i?bw:side,side);lv_obj_set_style_radius(button,side/6,0);
  lv_obj_set_style_bg_color(button,lv_color_hex(i?0x284458:0x9D3434),0);lv_obj_set_style_bg_color(button,lv_color_hex(0xB88308),LV_STATE_CHECKED);
  lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);lv_obj_set_style_shadow_width(button,0,0);
  lv_obj_set_style_bg_opa(button,LV_OPA_50,LV_STATE_DISABLED);
  lv_obj_set_style_bg_color(button,lv_color_hex(0x35799B),LV_STATE_PRESSED);
  if(i)lv_obj_set_pos(button,edge+(i-1)*(bw+edge),edge);else lv_obj_align(button,LV_ALIGN_RIGHT_MID,-edge,0);
  lv_obj_add_event_cb(button,cameraUiButton,LV_EVENT_CLICKED,&u);
  auto *label=lv_label_create(button);const lv_font_t *font=i<2?&ui_font_xlcdmin:&ui_font_text_24;
  lv_obj_set_style_text_font(label,font,0);lv_label_set_text(label,symbols[i]);
  if(i>=2)ui_text_scale(label,font,ui_font_xlcdmin.line_height*256/font->line_height);
  ui_layout_ink_place(label,0,0,i?bw:side,side);lv_obj_set_style_text_color(label,lv_color_white(),0);
 }
}
inline void cameraUiTick(CameraUi &u,uint32_t now,bool live,bool paused,bool light,bool online){
 if(!u.overlay)return;
 if(u.paused!=paused){auto *button=u.buttons[2];lv_obj_update_layout(button);auto *icon=lv_obj_get_child(button,0);lv_label_set_text(icon,paused?LV_SYMBOL_PLAY:LV_SYMBOL_PAUSE);ui_layout_ink_place(icon,0,0,lv_obj_get_width(button),lv_obj_get_height(button));}
 u.live=live;u.paused=paused;
 for(unsigned i:{1u,2u}){bool checked=i==1?light:paused;if(checked)lv_obj_add_state(u.buttons[i],LV_STATE_CHECKED);else lv_obj_clear_state(u.buttons[i],LV_STATE_CHECKED);}
 for(unsigned i:{1u,2u}){bool enabled=i==1?online:live;if(enabled)lv_obj_clear_state(u.buttons[i],LV_STATE_DISABLED);else lv_obj_add_state(u.buttons[i],LV_STATE_DISABLED);}
 if(!live||paused){if(!u.visible)cameraUiShow(u,true,now);}else if(u.visible&&uint32_t(now-u.touched)>=4000)cameraUiShow(u,false,now);
}
