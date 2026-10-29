#include <stdio.h>
#include "ui_helpers.h"

void _ui_bar_set_property(lv_obj_t *target, int id, int val)
{
   if (id == _UI_BAR_PROPERTY_VALUE_WITH_ANIM)
      lv_bar_set_value(target, val, LV_ANIM_ON);
   if (id == _UI_BAR_PROPERTY_VALUE)
      lv_bar_set_value(target, val, LV_ANIM_OFF);
}

void _ui_basic_set_property(lv_obj_t *target, int id, int val)
{
   if (id == _UI_BASIC_PROPERTY_POSITION_X)
      lv_obj_set_x(target, val);
   if (id == _UI_BASIC_PROPERTY_POSITION_Y)
      lv_obj_set_y(target, val);
   if (id == _UI_BASIC_PROPERTY_WIDTH)
      lv_obj_set_width(target, val);
   if (id == _UI_BASIC_PROPERTY_HEIGHT)
      lv_obj_set_height(target, val);
}

void _ui_dropdown_set_property(lv_obj_t *target, int id, int val)
{
   if (id == _UI_DROPDOWN_PROPERTY_SELECTED)
      lv_dropdown_set_selected(target, val);
}

void _ui_image_set_property(lv_obj_t *target, int id, uint8_t *val)
{
   if (id == _UI_IMAGE_PROPERTY_IMAGE)
      lv_img_set_src(target, val);
}

void _ui_label_set_property(lv_obj_t *target, int id, const char *val)
{
   if (id == _UI_LABEL_PROPERTY_TEXT)
      lv_label_set_text(target, val);
}

void _ui_slider_set_property(lv_obj_t *target, int id, int val)
{
   if (id == _UI_SLIDER_PROPERTY_VALUE_WITH_ANIM)
      lv_slider_set_value(target, val, LV_ANIM_ON);
   if (id == _UI_SLIDER_PROPERTY_VALUE)
      lv_slider_set_value(target, val, LV_ANIM_OFF);
}

void _ui_screen_change(lv_obj_t *target, lv_scr_load_anim_t fademode, int spd, int delay)
{
   lv_scr_load_anim(target, fademode, spd, delay, false);
}

void _ui_bar_increment(lv_obj_t *target, int val, int anm)
{
   int old = lv_bar_get_value(target);
   lv_bar_set_value(target, old + val, anm);
}

void _ui_slider_increment(lv_obj_t *target, int val, int anm)
{
   int old = lv_slider_get_value(target);
   lv_slider_set_value(target, old + val, anm);
   lv_obj_send_event(target, LV_EVENT_VALUE_CHANGED, 0);
}

void _ui_flag_modify(lv_obj_t *target, int32_t flag, int value)
{
   if (value == _UI_MODIFY_FLAG_TOGGLE)
   {
      if (lv_obj_has_flag(target, flag))
         lv_obj_clear_flag(target, flag);
      else
         lv_obj_add_flag(target, flag);
   }
   else if (value == _UI_MODIFY_FLAG_ADD)
      lv_obj_add_flag(target, flag);
   else
      lv_obj_clear_flag(target, flag);
}
void _ui_state_modify(lv_obj_t *target, int32_t state, int value)
{
   if (value == _UI_MODIFY_STATE_TOGGLE)
   {
      if (lv_obj_has_state(target, state))
         lv_obj_clear_state(target, state);
      else
         lv_obj_add_state(target, state);
   }
   else if (value == _UI_MODIFY_STATE_ADD)
      lv_obj_add_state(target, state);
   else
      lv_obj_clear_state(target, state);
}

void _ui_opacity_set(lv_obj_t *target, int val)
{
   lv_obj_set_style_opa(target, val, 0);
}

void _ui_anim_callback_free_user_data(lv_anim_t *a)
{
   lv_mem_free(a->user_data);
   a->user_data = NULL;
}

void _ui_anim_callback_set_x(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_obj_set_x(usr->target, v);
}

void _ui_anim_callback_set_y(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_obj_set_y(usr->target, v);
}

void _ui_anim_callback_set_width(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_obj_set_width(usr->target, v);
}

void _ui_anim_callback_set_height(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_obj_set_height(usr->target, v);
}

void _ui_anim_callback_set_opacity(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_obj_set_style_opa(usr->target, v, 0);
}

void _ui_anim_callback_set_image_zoom(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_img_set_zoom(usr->target, v);
}

void _ui_anim_callback_set_image_angle(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   lv_img_set_angle(usr->target, v);
}

void _ui_anim_callback_set_image_frame(lv_anim_t *a, int32_t v)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   usr->val = v;
   if (v < 0)
      v = 0;
   if (v >= usr->imgset_size)
      v = usr->imgset_size - 1;
   lv_img_set_src(usr->target, usr->imgset[v]);
}

int32_t _ui_anim_callback_get_x(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_obj_get_x_aligned(usr->target);
}

int32_t _ui_anim_callback_get_y(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_obj_get_y_aligned(usr->target);
}

int32_t _ui_anim_callback_get_width(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_obj_get_width(usr->target);
}

int32_t _ui_anim_callback_get_height(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_obj_get_height(usr->target);
}

int32_t _ui_anim_callback_get_opacity(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_obj_get_style_opa(usr->target,(lv_part_t)(0));
}

int32_t _ui_anim_callback_get_image_zoom(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_img_get_zoom(usr->target);
}

int32_t _ui_anim_callback_get_image_angle(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return lv_img_get_angle(usr->target);
}

int32_t _ui_anim_callback_get_image_frame(lv_anim_t *a)
{
   ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
   return usr->val;
}

void _ui_slider_set_text_value(lv_obj_t *trg, lv_obj_t *src, const char *prefix, const char *postfix)
{
   char buf[_UI_TEMPORARY_STRING_BUFFER_SIZE];
   lv_snprintf(buf, sizeof(buf), "%s%d%s", prefix, (int)lv_slider_get_value(src), postfix);
   lv_label_set_text(trg, buf);
}
void _ui_checked_set_text_value(lv_obj_t *trg, lv_obj_t *src, const char *txt_on, const char *txt_off)
{
   if (lv_obj_has_state(src, LV_STATE_CHECKED))
      lv_label_set_text(trg, txt_on);
   else
      lv_label_set_text(trg, txt_off);
}

lv_color_t _ui_get_complementary_color(lv_color_t color)
{
   return lv_color_brightness(color)>186?lv_color_black():lv_color_white();
}

void _ui_seconds_to_timeleft(uint32_t seconds, char *ret)
{
   int days = seconds / 86400;
   seconds %= 86400;
   int hours = seconds / 3600;
   seconds %= 3600;
   int minutes = seconds / 60;
   seconds %= 60;

   if (days > 0)
   {
      sprintf(ret, "%02dd %02dh %02dm", days, hours, minutes);
   }
   else if (hours > 0)
   {
      sprintf(ret, "%02dh %02dm", hours, minutes);
   }
   else
   {
      sprintf(ret, "%02dm", minutes);
   }
}
/* Shared native-pixel button layout. Keep one implementation in the binary. */
#undef lv_obj_get_coords
#undef lv_obj_get_content_coords
#undef lv_obj_get_width
#undef lv_obj_get_height
#undef lv_obj_set_pos
#undef lv_obj_set_size
#undef lv_obj_set_width
#undef lv_obj_set_height
#undef lv_text_get_size
/* A caption's line box can be centered while accented/CJK ink is not.
 * Measure against the full button without moving its background or hit target. */
static void ui_layout_caption(lv_obj_t *button){
 static bool busy=false;if(busy||lv_obj_get_child_count(button)!=1)return;
 lv_obj_t *label=lv_obj_get_child(button,0);if(!lv_obj_check_type(label,&lv_label_class))return;
 busy=true;ui_text_guard(label);
 const lv_font_t *font=lv_obj_get_style_text_font(label,LV_PART_MAIN);const char *text=lv_label_get_text(label);
 lv_area_t outer,inner,ink=ui_layout_ink(font,text);lv_obj_get_coords(button,&outer);lv_obj_get_content_coords(button,&inner);
 if(ink.x2>=ink.x1&&lv_area_get_width(&outer)>0){
  lv_point_t size;lv_text_get_size(&size,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  int padding=lv_obj_get_style_pad_left(label,LV_PART_MAIN)+lv_obj_get_style_pad_right(label,LV_PART_MAIN);
  int maximum=LV_MAX(1,lv_area_get_width(&outer)-2*ui_specs_get().inset);
  bool scroll=size.x+padding>maximum;
  lv_label_long_mode_t mode=scroll?LV_LABEL_LONG_SCROLL:LV_LABEL_LONG_CLIP;
  if(lv_label_get_long_mode(label)!=mode)lv_label_set_long_mode(label,mode);
  lv_obj_add_flag(label,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_align(label,LV_ALIGN_TOP_LEFT);
  lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
  lv_obj_set_size(label,scroll?maximum:size.x+padding,font->line_height+lv_obj_get_style_pad_top(label,LV_PART_MAIN)+lv_obj_get_style_pad_bottom(label,LV_PART_MAIN));
  int x=outer.x1-inner.x1,y=outer.y1-inner.y1,w=lv_area_get_width(&outer),h=lv_area_get_height(&outer);
  if(scroll)lv_obj_set_pos(label,x+(w-maximum)/2,y+(h-lv_area_get_height(&ink))/2-ink.y1-lv_obj_get_style_pad_top(label,LV_PART_MAIN));
  else ui_layout_ink_place(label,x,y,w,h);
 }
 busy=false;
}
static void ui_layout_caption_event(lv_event_t *e){
 lv_obj_t *obj=lv_event_get_target_obj(e);ui_layout_caption(lv_obj_check_type(obj,&lv_label_class)?lv_obj_get_parent(obj):obj);
}
void ui_layout_captions_tree(lv_obj_t *root){
 if(lv_obj_check_type(root,&lv_button_class)&&lv_obj_get_child_count(root)==1&&lv_obj_check_type(lv_obj_get_child(root,0),&lv_label_class)&&!lv_obj_has_flag(lv_obj_get_child(root,0),LV_OBJ_FLAG_IGNORE_LAYOUT)){
  lv_obj_t *label=lv_obj_get_child(root,0);
  lv_obj_remove_event_cb(root,ui_layout_caption_event);lv_obj_remove_event_cb(label,ui_layout_caption_event);
  lv_obj_add_event_cb(root,ui_layout_caption_event,LV_EVENT_SIZE_CHANGED,NULL);
  lv_obj_add_event_cb(root,ui_layout_caption_event,LV_EVENT_STYLE_CHANGED,NULL);
  lv_obj_add_event_cb(label,ui_layout_caption_event,LV_EVENT_SIZE_CHANGED,NULL);
  ui_layout_caption(root);
 }
 for(unsigned i=0;i<lv_obj_get_child_count(root);++i)ui_layout_captions_tree(lv_obj_get_child(root,i));
}

void ui_specs_fill_remaining(lv_obj_t *item,bool vertical) {
 /* Allocate flexible native-pixel space after fixed siblings and gaps.
  * Scaling a reference height independently loses fractional pixels. */
 if(vertical)lv_obj_set_height(item,0);else lv_obj_set_width(item,0);
 lv_obj_set_flex_grow(item,1);
}

ui_specs_t ui_specs_get(void) {
 ui_chrome_metrics_t chrome=ui_chrome_metrics(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL));
 int ref=chrome.reference_height;
 return (ui_specs_t){LV_MAX(1,ui_chrome_px(ref,1)),ui_chrome_px(ref,4),
  ui_chrome_px(ref,3),ui_chrome_px(ref,6),chrome.control,chrome.icon};
}

void ui_specs_text_window(lv_obj_t *label,int width) {
 ui_text_guard(label);
 const lv_font_t *font=lv_obj_get_style_text_font(label,LV_PART_MAIN);
 int pad=lv_obj_get_style_pad_left(label,LV_PART_MAIN)+lv_obj_get_style_pad_right(label,LV_PART_MAIN);
 lv_point_t size;lv_text_get_size(&size,lv_label_get_text(label),font,
  lv_obj_get_style_text_letter_space(label,LV_PART_MAIN),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 lv_label_long_mode_t mode=size.x+pad>width?LV_LABEL_LONG_SCROLL:LV_LABEL_LONG_CLIP;
 if(lv_label_get_long_mode(label)!=mode)lv_label_set_long_mode(label,mode);
 (lv_obj_set_width)(label,LV_MAX(1,width));
 (lv_obj_set_height)(label,font->line_height+lv_obj_get_style_pad_top(label,LV_PART_MAIN)+lv_obj_get_style_pad_bottom(label,LV_PART_MAIN));
}

void ui_layout_action_fit(lv_obj_t *button,int width,int height,int pad,int gap) {
 if(lv_obj_get_child_count(button)!=2)return;
 lv_obj_t *icon=lv_obj_get_child(button,0),*label=lv_obj_get_child(button,1);
 if(!lv_obj_check_type(icon,&lv_label_class)||!lv_obj_check_type(label,&lv_label_class))return;
 ui_text_scale(icon,ui_text_source(lv_obj_get_style_text_font(icon,LV_PART_MAIN)),ui_layout_base_zoom());
 ui_text_scale(label,ui_text_source(lv_obj_get_style_text_font(label,LV_PART_MAIN)),ui_layout_base_zoom());
 ui_text_guard(icon);ui_text_guard(label);lv_obj_clear_flag(icon,LV_OBJ_FLAG_HIDDEN);
 lv_obj_set_size(icon,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
 lv_area_t ia=ui_layout_ink(lv_obj_get_style_text_font(icon,LV_PART_MAIN),lv_label_get_text(icon));
 lv_area_t ta=ui_layout_ink(lv_obj_get_style_text_font(label,LV_PART_MAIN),lv_label_get_text(label));
 int iw=lv_area_get_width(&ia),tw=lv_area_get_width(&ta);
 int available=LV_MAX(1,width-2*pad-iw-gap);
 lv_point_t size;lv_text_get_size(&size,lv_label_get_text(label),lv_obj_get_style_text_font(label,LV_PART_MAIN),0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 int textPadding=lv_obj_get_style_pad_left(label,LV_PART_MAIN)+lv_obj_get_style_pad_right(label,LV_PART_MAIN);
 bool overflow=size.x+textPadding>available;
 int visible=overflow?available:tw;
 int start=overflow?pad:(width-iw-gap-visible)/2;
 ui_layout_ink_place(icon,start,0,iw,height);
 lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
 ui_specs_text_window(label,overflow?available:size.x+textPadding);
 if(overflow)lv_obj_set_pos(label,start+iw+gap,(height-lv_area_get_height(&ta))/2-ta.y1-lv_obj_get_style_pad_top(label,LV_PART_MAIN));
 else ui_layout_ink_place(label,start+iw+gap,0,visible,height);
}

ui_material_content_t ui_specs_material_content(lv_area_t card) {
 ui_specs_t m=ui_specs_get();
 int swatchInset=LV_MAX(m.inset,ui_chrome_px(ui_chrome_metrics(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL)).reference_height,8));
 int top=card.y1+m.inset+m.unit;
 ui_material_content_t result;
 result.swatch=(lv_area_t){card.x1+swatchInset,top,card.x2-swatchInset,top+LV_MAX(2,m.gap*2)-1};
 int first=result.swatch.y2+1+m.inset,last=card.y2-m.inset;
 int middle=first+(last-first+1)/2;
 result.captions[0]=(lv_area_t){card.x1+m.inset,first,card.x2-m.inset,middle-1};
 result.captions[1]=(lv_area_t){card.x1+m.inset,middle,card.x2-m.inset,last};
 return result;
}

/* Two-line setup cards use native flex geometry, fixed icons and text-only scrolling. */
#if XTOUCH_UI_HIDPI_ASSETS
LV_FONT_DECLARE(ui_font_setup_symbols_36);
LV_FONT_DECLARE(ui_font_setup_symbols_48);
#endif
void ui_layout_setup_choice_content(lv_obj_t *card,const char *symbol,const char *title,const char *detail){
 lv_obj_clean(card);int inset=ui_specs_get().inset,gap=ui_specs_get().gap;
 lv_obj_set_style_pad_all(card,inset,0);lv_obj_set_style_pad_column(card,gap,0);
 lv_obj_set_flex_flow(card,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(card,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
 lv_obj_update_layout(card);
 lv_obj_t *slot=lv_obj_create(card);lv_obj_remove_style_all(slot);
 int side=LV_MAX(16,lv_obj_get_height(card)-2*inset);(lv_obj_set_size)(slot,side,side);
 lv_obj_clear_flag(slot,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *icon=lv_label_create(slot);lv_label_set_text(icon,symbol);
 const lv_font_t *symbolFont=&ui_font_symbols_28;
#if XTOUCH_UI_HIDPI_ASSETS
 symbolFont=lv_disp_get_ver_res(NULL)>=400?&ui_font_setup_symbols_48:&ui_font_setup_symbols_36;
#endif
 (lv_obj_set_style_text_font)(icon,symbolFont,0);lv_obj_set_style_text_color(icon,lv_color_hex(UI_COLOR_ACTION_POSITIVE),0);
 lv_obj_add_event_cb(slot,ui_layout_setup_symbol_event,LV_EVENT_SIZE_CHANGED,NULL);
 lv_obj_t *column=lv_obj_create(card);lv_obj_remove_style_all(column);(lv_obj_set_width)(column,0);(lv_obj_set_height)(column,lv_pct(100));lv_obj_set_flex_grow(column,1);
 lv_obj_clear_flag(column,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_flex_flow(column,LV_FLEX_FLOW_COLUMN);lv_obj_set_flex_align(column,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER);lv_obj_set_style_pad_row(column,gap/2,0);
 for(unsigned n=0;n<2;++n){lv_obj_t *text=lv_label_create(column);lv_label_set_text(text,n?detail:title);(lv_obj_set_width)(text,lv_pct(100));ui_text_scale(text,LV_FONT_DEFAULT,ui_layout_base_zoom()*(n?7:8)/8);ui_text_guard(text);lv_obj_set_style_text_color(text,lv_color_hex(n?0xAAAAAA:0xFFFFFF),0);lv_obj_add_event_cb(text,ui_layout_single_line_event,LV_EVENT_SIZE_CHANGED,NULL);}
 lv_obj_update_layout(card);lv_obj_send_event(slot,LV_EVENT_SIZE_CHANGED,NULL);
 for(unsigned n=0;n<2;++n){lv_obj_t *text=lv_obj_get_child(column,n);ui_specs_text_window(text,lv_obj_get_content_width(column));}
}
