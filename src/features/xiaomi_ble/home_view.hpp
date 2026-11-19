#pragma once
#include "ui/layout_shared.h"
#include <cstdio>
#include <cstring>
#include <new>

/* Shared by firmware and the native LVGL preview; the provider owns radio/config
 * synchronization. This view only controls presentation and object lifetimes. */
namespace HomeEnvironment {
constexpr uint32_t freshMs=600000;
struct Options {
 bool configured;
 uint8_t mode;
 uint8_t interval;
 bool hasAms;
};
struct Provider {
 Options (*options)();
 bool (*reading)(float &,float &,uint32_t &);
 void (*visibility)(bool);
};
struct View {
 lv_obj_t *strip,*slot,*label,*clock;
 uint32_t born;
 bool showing=false;
 Provider provider;
};
inline void update(lv_timer_t *timer){
 auto *view=static_cast<View *>(lv_timer_get_user_data(timer));
 const auto options=view->provider.options();
 float temperature=0,humidity=0;uint32_t age=0;
 bool available=options.configured&&options.mode&&view->provider.reading(temperature,humidity,age);
 bool visible=available&&(options.mode==2||!options.hasAms||
  ((lv_tick_get()-view->born)/(uint32_t(options.interval)*1000))%2==1);
 if(visible){
  const auto *font=lv_obj_get_style_text_font(view->clock,LV_PART_MAIN);
  lv_obj_set_style_text_font(view->label,font,0);
  char text[40];std::snprintf(text,sizeof(text),"%s%.1f℃ %.0f%%",age>freshMs?"~":"",double(temperature),double(humidity));
  if(std::strcmp(lv_label_get_text(view->label),text))lv_label_set_text(view->label,text);
  lv_point_t extent;lv_text_get_size(&extent,text,font,lv_obj_get_style_text_letter_space(view->clock,LV_PART_MAIN),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  lv_obj_set_width(view->slot,extent.x);
  lv_obj_add_flag(view->strip,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(view->slot,LV_OBJ_FLAG_HIDDEN);
 }else{
  lv_obj_add_flag(view->slot,LV_OBJ_FLAG_HIDDEN);
  if(view->showing)lv_obj_remove_flag(view->strip,LV_OBJ_FLAG_HIDDEN);
 }
 ui_layout_status_text(view->label);view->showing=visible;
 view->provider.visibility(visible);
}
inline void removed(lv_event_t *event){
 auto *timer=static_cast<lv_timer_t *>(lv_event_get_user_data(event));
 delete static_cast<View *>(lv_timer_get_user_data(timer));lv_timer_delete(timer);
}
inline void bind(lv_obj_t *strip,lv_obj_t *clockGap,Provider provider){
 auto *view=new(std::nothrow) View;if(!view)return;
 view->provider=provider;view->strip=strip;view->clock=lv_obj_get_child(clockGap,0);view->born=lv_tick_get();
 view->slot=lv_obj_create(lv_obj_get_parent(strip));lv_obj_remove_style_all(view->slot);
 lv_obj_set_size(view->slot,0,lv_pct(100));lv_obj_remove_flag(view->slot,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_add_flag(view->slot,LV_OBJ_FLAG_HIDDEN);lv_obj_set_flex_grow(view->slot,0);
 view->label=lv_label_create(view->slot);lv_label_set_text(view->label,"");
 lv_obj_add_flag(view->label,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_remove_flag(view->label,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_set_style_text_font(view->label,lv_obj_get_style_text_font(view->clock,LV_PART_MAIN),0);
 lv_obj_set_style_text_letter_space(view->label,lv_obj_get_style_text_letter_space(view->clock,LV_PART_MAIN),0);
 lv_obj_set_style_text_color(view->label,lv_color_hex(UI_COLOR_BLUE),0);lv_obj_align(view->label,LV_ALIGN_CENTER,0,0);
 auto *timer=lv_timer_create(update,1000,view);
 if(!timer){lv_obj_delete(view->slot);delete view;return;}
 lv_obj_add_event_cb(strip,removed,LV_EVENT_DELETE,timer);update(timer);
}
}
