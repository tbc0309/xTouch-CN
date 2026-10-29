#include "settings_dropdown.h"
#include "features/xiaomi_ble/display_preferences.h"

lv_obj_t *ui_settings_dropdown_row(lv_obj_t *parent,const char *title,lv_obj_t **caption,lv_obj_t **select){
 lv_obj_t *row=lv_obj_create(parent);lv_obj_remove_style_all(row);
 lv_obj_set_size(row,lv_pct(100),48);lv_obj_set_style_bg_color(row,lv_color_hex(0x222222),0);
 lv_obj_set_style_bg_opa(row,LV_OPA_COVER,0);lv_obj_set_style_radius(row,6,0);
 lv_obj_set_style_pad_left(row,16,0);lv_obj_set_style_pad_right(row,16,0);
 lv_obj_set_style_pad_column(row,8,0);
 lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(row,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
 lv_obj_t *label=lv_label_create(row);lv_label_set_text(label,title);
 lv_obj_set_width(label,0);lv_obj_set_flex_grow(label,1);lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
 lv_obj_add_event_cb(label,ui_layout_single_line_event,LV_EVENT_SIZE_CHANGED,NULL);
 lv_obj_t *dropdown=lv_dropdown_create(row);lv_obj_set_size(dropdown,112,34);
 lv_obj_set_style_bg_color(dropdown,lv_color_hex(0x333333),0);lv_obj_set_style_text_color(dropdown,lv_color_white(),0);
 lv_obj_set_style_border_width(dropdown,0,0);lv_obj_set_style_pad_ver(dropdown,6,0);
 lv_obj_set_style_text_color(dropdown,lv_color_hex(0x888888),LV_STATE_DISABLED);
 lv_obj_set_style_text_color(dropdown,lv_color_hex(0x888888),LV_PART_INDICATOR|LV_STATE_DISABLED);
 lv_obj_t *list=lv_dropdown_get_list(dropdown);lv_obj_set_style_bg_color(list,lv_color_hex(0x333333),0);
 lv_obj_set_style_text_color(list,lv_color_white(),0);lv_obj_set_style_bg_color(list,lv_color_hex(0x555555),LV_PART_SELECTED|LV_STATE_CHECKED);
 ui_layout_dropdown(dropdown);if(caption)*caption=label;if(select)*select=dropdown;return row;
}
static void refresh(lv_timer_t *timer){
 lv_obj_t *select=lv_timer_get_user_data(timer);uint8_t mode=0;bool bound=xtouch_environment_display_get(&mode);
 if(!bound){lv_dropdown_close(select);lv_obj_add_state(select,LV_STATE_DISABLED);}
 else lv_obj_clear_state(select,LV_STATE_DISABLED);
 if(lv_dropdown_get_selected(select)!=mode)lv_dropdown_set_selected(select,mode);
}
static void changed(lv_event_t *event){
 lv_obj_t *select=lv_event_get_target_obj(event);
 if(lv_obj_has_state(select,LV_STATE_DISABLED))return;
 xtouch_environment_display_request(lv_dropdown_get_selected(select));
}
static void deleted(lv_event_t *event){lv_timer_del(lv_event_get_user_data(event));}
lv_obj_t *ui_environment_setting_create(lv_obj_t *parent,lv_obj_t **caption,lv_obj_t **select){
 lv_obj_t *dropdown,*row=ui_settings_dropdown_row(parent,xtouch_local("温湿度计","Thermometer"),caption,&dropdown);
 lv_dropdown_set_options(dropdown,xtouch_local("关闭显示\n交替显示\n仅温湿度","Hide\nAlternate\nSensor only"));
 ui_layout_dropdown(dropdown);lv_obj_add_event_cb(dropdown,changed,LV_EVENT_VALUE_CHANGED,NULL);
 lv_timer_t *timer=lv_timer_create(refresh,500,dropdown);
 if(timer){lv_obj_add_event_cb(dropdown,deleted,LV_EVENT_DELETE,timer);refresh(timer);}
 else lv_obj_add_state(dropdown,LV_STATE_DISABLED);
 if(select)*select=dropdown;return row;
}
