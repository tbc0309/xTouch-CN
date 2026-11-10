/* Shape composition operates in physical pixels and reuses the production
 * widgets, event handlers and message subscriptions. Resource size is separate. */
#include "ui/ui.h"
#include "shape.h"
#include "xtouch/hms.h"
#include "features/network/print_phase.h"
#include "src/core/lv_obj_private.h"
#include <stdlib.h>
#include <string.h>

ui_shape_area_t ui_shape_area(int w,int h,bool overlay){
 ui_shape_area_t a={0,0,w,h,LV_MAX(32,w/10),LV_MAX(4,w/70)};
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
 a.x=w*18/100;a.y=h*16/100;a.w=w*64/100;a.h=h*(overlay?68:62)/100;
 if(!overlay)a.h=LV_MIN(a.h,h*83/100-a.hit/2-a.gap-a.y);
#elif XTOUCH_UI_LAYOUT==UI_LAYOUT_SQUARE
 a.x=w*3/100;a.y=h*4/100;a.w=w-a.x*2;a.h=h*(overlay?92:80)/100;
#else
 (void)overlay;
#endif
 return a;
}
#if XTOUCH_UI_LAYOUT!=UI_LAYOUT_LANDSCAPE
static unsigned object_count(lv_obj_t *o){unsigned n=1;for(unsigned i=0;i<lv_obj_get_child_cnt(o);i++)n+=object_count(lv_obj_get_child(o,i));return n;}
typedef struct {lv_obj_t *object;unsigned grow;bool hidden,ignored;} flex_state;
static void release_walk(lv_obj_t *o,flex_state *states,unsigned *n){
 states[(*n)++]=(flex_state){o,lv_obj_get_style_flex_grow(o,0),lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN),lv_obj_has_flag(o,LV_OBJ_FLAG_IGNORE_LAYOUT)};
 lv_obj_clear_flag(o,LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_IGNORE_LAYOUT);
 lv_obj_set_flex_grow(o,0);lv_obj_mark_layout_as_dirty(o);
 for(unsigned i=0;i<lv_obj_get_child_cnt(o);i++)release_walk(lv_obj_get_child(o,i),states,n);
}
static void release_flex(lv_obj_t *o){
 /* LVGL retains flex-owned dimensions until the old layout gets a pass
  * with grow=0. Release that ownership before switching to fixed geometry.
  * Hidden editors participate in this layout-only pass; nothing is drawn. */
 flex_state *states=malloc(object_count(o)*sizeof(*states));if(!states)return;
 unsigned n=0;release_walk(o,states,&n);lv_obj_update_layout(o);
 for(unsigned i=0;i<n;i++){lv_obj_set_flex_grow(states[i].object,states[i].grow);if(states[i].hidden)lv_obj_add_flag(states[i].object,LV_OBJ_FLAG_HIDDEN);if(states[i].ignored)lv_obj_add_flag(states[i].object,LV_OBJ_FLAG_IGNORE_LAYOUT);}
 free(states);
}
static void box(lv_obj_t *o,int x,int y,int w,int h){
 /* LVGL 9.6 retains flex-owned dimensions after a hidden item is reparented.
  * This native compositor now owns both axes. Clear that ownership before
  * applying fixed bounds; style limits alone do not release it. */
 o->w_layout=0;o->h_layout=0;
 lv_obj_set_style_min_width(o,0,0);lv_obj_set_style_min_height(o,0,0);
 lv_obj_set_style_max_width(o,LV_MAX(1,w),0);lv_obj_set_style_max_height(o,LV_MAX(1,h),0);
 lv_obj_set_flex_grow(o,0);lv_obj_add_flag(o,LV_OBJ_FLAG_IGNORE_LAYOUT);
 lv_obj_set_align(o,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,LV_MAX(1,w),LV_MAX(1,h));
}
void ui_shape_place(lv_obj_t *o,int x,int y,int w,int h){box(o,x,y,w,h);}
static void plain(lv_obj_t *o){
 lv_obj_set_layout(o,0);lv_obj_set_style_pad_all(o,0,0);lv_obj_set_style_border_width(o,0,0);
 lv_obj_set_style_bg_opa(o,LV_OPA_TRANSP,0);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);
}
static void dropdown_safe_open(lv_event_t *e){
 lv_obj_t *o=lv_event_get_target_obj(e),*list=lv_dropdown_get_list(o);if(!list)return;
 bool navigation=ui_sidebarComponent&&lv_obj_get_screen(o)==lv_obj_get_screen(ui_sidebarComponent)&&!lv_obj_has_flag(ui_sidebarComponent,LV_OBJ_FLAG_HIDDEN);
 ui_shape_area_t safe=ui_shape_area(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL),!navigation);
 lv_obj_update_layout(o);
 lv_area_t bounds;lv_obj_get_coords(o,&bounds);
 int above=bounds.y1-safe.y-safe.gap,below=safe.y+safe.h-1-bounds.y2-safe.gap;
 lv_dropdown_set_dir(o,below>=above?LV_DIR_BOTTOM:LV_DIR_TOP);
 lv_obj_set_style_max_height(list,LV_MAX(44,LV_MAX(above,below)),0);
 lv_obj_set_style_max_width(list,safe.w,0);
}
void ui_shape_theme(lv_obj_t *o){
 /* Material swatches are data, not theme surfaces. */
 if(lv_obj_get_style_min_width(o,0)==14&&lv_obj_get_style_min_height(o,0)==14)return;
 /* Read the normal local role, never the currently resolved disabled/checked
  * colour. Reapplying a theme must not turn a primary action into a gray one. */
 lv_style_value_t base;
 uint32_t old=lv_color_to_int(lv_obj_get_style_bg_color(o,0));
 if(lv_obj_get_local_style_prop(o,LV_STYLE_BG_COLOR,&base,0)==LV_STYLE_RES_FOUND)old=lv_color_to_int(base.color);
 if(old==0x555555||old==0x666666||old==0x777777||old==0x444444||old==0x333333||old==0x222222){
  lv_obj_set_style_bg_color(o,lv_color_hex(old==0x333333?UI_COLOR_SURFACE:UI_COLOR_BUTTON),0);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_BUTTON_PRESSED),LV_STATE_PRESSED);
 }
 bool button=lv_obj_check_type(o,&lv_button_class);
 bool key=lv_obj_check_type(o,&lv_label_class)&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE)&&lv_obj_get_style_bg_opa(o,0)>0;
 if(button||key){
  uint32_t normal=UI_COLOR_BUTTON,pressed=UI_COLOR_BUTTON_PRESSED;
  if(old==UI_COLOR_BLUE||old==UI_COLOR_CONTROL_ACCENT){normal=UI_COLOR_CONTROL_ACCENT;pressed=UI_COLOR_CONTROL_ACCENT_PRESSED;}
  else if(old==UI_COLOR_ACTION_POSITIVE||old==UI_COLOR_CONTROL_SUCCESS){normal=UI_COLOR_CONTROL_SUCCESS;pressed=UI_COLOR_ACTION_POSITIVE_PRESSED;}
  else if(old==UI_COLOR_ACTION_DANGER){normal=UI_COLOR_ACTION_DANGER;pressed=UI_COLOR_ACTION_DANGER_PRESSED;}
  else if(old==UI_COLOR_BUTTON_SECONDARY)normal=UI_COLOR_BUTTON_SECONDARY;
  lv_obj_set_style_bg_color(o,lv_color_hex(normal),0);
  lv_obj_set_style_bg_color(o,lv_color_hex(pressed),LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_DISABLED),LV_STATE_DISABLED);
  lv_obj_set_style_text_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_STATE_DISABLED);
  lv_obj_set_style_bg_opa(o,LV_OPA_COVER,LV_STATE_DISABLED);
  lv_obj_set_style_opa(o,LV_OPA_60,LV_STATE_DISABLED);
 }
 /* Widget parts and states are explicit: a checked switch must never inherit
  * an old black knob or a different green from another settings factory. */
 if(lv_obj_check_type(o,&lv_switch_class)){
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_BUTTON_SECONDARY),LV_PART_MAIN);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_ACCENT),LV_PART_MAIN|LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_ACCENT),LV_PART_INDICATOR|LV_STATE_CHECKED);
  lv_obj_set_style_bg_opa(o,LV_OPA_COVER,LV_PART_INDICATOR|LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_PRIMARY),LV_PART_KNOB);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_PRIMARY),LV_PART_KNOB|LV_STATE_CHECKED);
  for(unsigned checked=0;checked<2;checked++){
   lv_state_t state=LV_STATE_DISABLED|(checked?LV_STATE_CHECKED:0);
   lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_DISABLED),LV_PART_MAIN|state);
   lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_DISABLED),LV_PART_INDICATOR|state);
   lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_PART_KNOB|state);
  }
 }
 /* Read-only print progress retains its telemetry colours. */
 if(lv_obj_check_type(o,&lv_slider_class)&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE)){
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_BUTTON_SECONDARY),LV_PART_MAIN);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_ACCENT),LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_PRIMARY),LV_PART_KNOB);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_PART_KNOB|LV_STATE_DISABLED);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_DISABLED),LV_PART_INDICATOR|LV_STATE_DISABLED);
 }
 if(lv_obj_check_type(o,&lv_dropdown_class)||lv_obj_check_type(o,&lv_textarea_class)){
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_BUTTON),0);
  lv_obj_set_style_text_color(o,lv_color_hex(UI_COLOR_TEXT_PRIMARY),0);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_CONTROL_DISABLED),LV_STATE_DISABLED);
  lv_obj_set_style_text_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_STATE_DISABLED);
  if(lv_obj_check_type(o,&lv_textarea_class))lv_obj_set_style_text_color(o,lv_color_hex(UI_COLOR_TEXT_SECONDARY),LV_PART_TEXTAREA_PLACEHOLDER);
  if(lv_obj_check_type(o,&lv_dropdown_class)){
   lv_obj_remove_event_cb(o,dropdown_safe_open);
   lv_obj_add_event_cb(o,dropdown_safe_open,LV_EVENT_READY,NULL);
   lv_obj_set_style_text_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_PART_INDICATOR|LV_STATE_DISABLED);
   lv_obj_t *list=lv_dropdown_get_list(o);
   if(list){lv_obj_set_style_bg_color(list,lv_color_hex(UI_COLOR_SURFACE),0);lv_obj_set_style_text_color(list,lv_color_hex(UI_COLOR_TEXT_PRIMARY),0);lv_obj_set_style_border_color(list,lv_color_hex(UI_COLOR_BORDER),0);lv_obj_set_style_bg_color(list,lv_color_hex(UI_COLOR_CONTROL_ACCENT),LV_PART_SELECTED|LV_STATE_CHECKED);lv_obj_set_style_text_color(list,lv_color_hex(UI_COLOR_TEXT_PRIMARY),LV_PART_SELECTED|LV_STATE_CHECKED);}
  }
 }
 uint32_t border=lv_color_to_int(lv_obj_get_style_border_color(o,0));
 if(border>=0x222222&&border<0xFFFFFF&&((border>>16)&255)==((border>>8)&255)&&((border>>8)&255)==(border&255))lv_obj_set_style_border_color(o,lv_color_hex(UI_COLOR_BORDER),0);
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_SCROLLABLE)){
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_TEXT_DISABLED),LV_PART_SCROLLBAR|LV_STATE_SCROLLED);
 }
 for(unsigned i=0;i<lv_obj_get_child_cnt(o);i++)ui_shape_theme(lv_obj_get_child(o,i));
}
static lv_obj_t *child(lv_obj_t *c,unsigned i){return ui_comp_get_child(c,i);}
static void move(lv_obj_t *o,lv_obj_t *parent,int x,int y,int w,int h){lv_obj_set_parent(o,parent);box(o,x,y,w,h);}
static void compact_text(lv_obj_t *o,int height){
 if(lv_obj_check_type(o,&lv_label_class)||lv_obj_check_type(o,&lv_textarea_class)){
  const lv_font_t *f=ui_text_source(lv_obj_get_style_text_font(o,0));
  ui_text_scale(o,f,height*256/LV_MAX(1,f->line_height));
 }
 for(unsigned i=0;i<lv_obj_get_child_cnt(o);i++)compact_text(lv_obj_get_child(o,i),height);
}
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
static void print_ring(lv_event_t *e){
 lv_obj_t *root=lv_event_get_target_obj(e);
 if(lv_event_get_code(e)==LV_EVENT_MSG_RECEIVED){lv_obj_invalidate(root);return;}
 if(lv_event_get_code(e)!=LV_EVENT_DRAW_MAIN||!xtouch_printer_online())return;
 int status=bambuStatus.print_status;
 hms_severity_t severity=hms_severity();
 bool active=status==XTOUCH_PRINT_STATUS_RUNNING||status==XTOUCH_PRINT_STATUS_PREPARE||status==XTOUCH_PRINT_STATUS_PAUSED;
 if(!active&&status!=XTOUCH_PRINT_STATUS_FAILED&&severity==HMS_SEVERITY_INFO)return;
 bool preparing=xtouch_print_preparing(status==XTOUCH_PRINT_STATUS_PREPARE,status==XTOUCH_PRINT_STATUS_RUNNING,bambuStatus.current_layer_valid,bambuStatus.current_layer);
 /* Wait for first-layer evidence at 0%; positive progress still supports
  * printer firmware which never reports layer counters. */
 if(status==XTOUCH_PRINT_STATUS_RUNNING&&!bambuStatus.current_layer_valid&&bambuStatus.mc_print_percent<=0)preparing=true;
 uint32_t color=(status==XTOUCH_PRINT_STATUS_FAILED||severity==HMS_SEVERITY_ERROR)?UI_COLOR_ERROR:
  (severity==HMS_SEVERITY_WARNING||status==XTOUCH_PRINT_STATUS_PAUSED||preparing)?UI_COLOR_WARNING:0x00FF00;
 int percent=status==XTOUCH_PRINT_STATUS_PREPARE?0:LV_CLAMP(0,bambuStatus.mc_print_percent,100);
 lv_area_t bounds;lv_obj_get_coords(root,&bounds);
 lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);
 d.center=(lv_point_t){(bounds.x1+bounds.x2+1)/2,(bounds.y1+bounds.y2+1)/2};
 d.radius=LV_MIN(lv_area_get_width(&bounds),lv_area_get_height(&bounds))/2-1;
 d.width=LV_MAX(2,lv_area_get_width(&bounds)/150);d.color=lv_color_hex(color);
 d.start_angle=270;d.end_angle=630;d.opa=LV_OPA_20;
 lv_layer_t *layer=lv_event_get_layer(e);lv_draw_arc(layer,&d);
 if(percent>0){d.end_angle=270+percent*360/100;d.opa=LV_OPA_COVER;lv_draw_arc(layer,&d);}
}
static void ring_bind(lv_obj_t *root){
 lv_obj_remove_event_cb(root,print_ring);
 lv_obj_add_event_cb(root,print_ring,LV_EVENT_DRAW_MAIN,NULL);
 lv_obj_add_event_cb(root,print_ring,LV_EVENT_MSG_RECEIVED,NULL);
 const unsigned messages[]={XTOUCH_ON_PRINT_STATUS,XTOUCH_ON_ERROR,XTOUCH_ON_MQTT,XTOUCH_ON_MODEL};
 for(unsigned i=0;i<sizeof(messages)/sizeof(messages[0]);i++)lv_msg_subsribe_obj(messages[i],root,NULL);
}
#endif
static void nav(lv_obj_t *root,lv_obj_t *n,int w,int h){
 plain(root);plain(n);box(n,0,0,w,h);lv_obj_move_foreground(n);
 lv_obj_clear_flag(n,LV_OBJ_FLAG_CLICKABLE);
 static const unsigned index[]={1,3,5,7,9};
 static const int arc[][2]={{12,66},{27,83},{50,94},{73,83},{88,66}};
 const int hit=LV_MAX(32,w/10);
 for(unsigned i=0;i<5;i++){
  lv_obj_t *b=child(n,index[i]);
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
  box(b,w*arc[i][0]/100-hit/2,h*arc[i][1]/100-hit/2,hit,hit);
#else
  box(b,w*(2*i+1)/10-hit/2,h*94/100-hit/2,hit,hit);
#endif
  lv_obj_set_style_bg_color(b,lv_color_hex(UI_COLOR_NAV),0);
  lv_obj_set_style_bg_color(b,lv_color_hex(UI_COLOR_NAV_ACTIVE),LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(b,lv_color_hex(UI_COLOR_NAV_PRESSED),LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(b,lv_color_hex(UI_COLOR_NAV_PRESSED),LV_STATE_CHECKED|LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(b,LV_OPA_COVER,0);lv_obj_set_style_radius(b,LV_RADIUS_CIRCLE,0);
  lv_obj_set_style_border_width(b,1,0);
  lv_obj_set_style_border_color(b,lv_color_hex(UI_COLOR_NAV_ACTIVE),0);
  if(lv_obj_get_child_cnt(b))lv_obj_center(lv_obj_get_child(b,0));
 }
}
static void home_connection(lv_event_t *e){ui_shape_home_state(lv_event_get_target_obj(e));}
static void home(lv_obj_t *c,ui_shape_area_t a){
 const int w=a.w,h=a.h,g=a.gap,hit=a.hit;
 lv_obj_add_flag(c,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
 lv_obj_t *left=child(c,1),*right=child(c,26);plain(left);plain(right);
 box(left,0,0,w,h);box(right,0,0,w,h);
 lv_obj_t *device=child(c,39);move(device,c,0,0,w,hit*3/4); // Device identity retains its picker action.
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
 int sw=lv_disp_get_hor_res(NULL),sh=lv_disp_get_ver_res(NULL);
 move(device,lv_obj_get_parent(c),sw*26/100,sh*6/100+sh/75+1,sw*48/100,hit*3/4);
#endif
 /* Release both the old column geometry and its per-label height limits.
  * Model and profile name share one vertically centred physical-pixel row. */
 lv_obj_set_layout(device,0);lv_obj_set_style_pad_all(device,0,0);
 int header=0;
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_SQUARE
 header=hit*3/4+g;
#endif
 move(child(c,2),c,0,header,w,hit/2);
 const int ty=header+hit/2+g,th=hit*3/4;
 lv_obj_t *temps=child(c,29);move(temps,c,0,ty,w,th);
 plain(temps);lv_obj_set_flex_flow(temps,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(temps,g,0);
 const unsigned ti[]={30,33,36};
 for(unsigned i=0;i<3;i++){
  lv_obj_t *t=child(c,ti[i]);plain(t);lv_obj_clear_flag(t,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_width(t,0);lv_obj_set_flex_grow(t,1);lv_obj_set_height(t,th);lv_obj_set_style_pad_column(t,4,0);
 }
 int y=ty+th+g,ph=LV_MAX(hit,h-y-hit-g);
 /* The image surface may extend beyond the text column. Rounded idle edges
  * leave room for the side navigation while keeping the model at full height. */
 const int imageOverhang=XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND?w/12:0;
 const int imageWidth=w+2*imageOverhang;
 move(child(c,10),lv_obj_get_parent(c),a.x-imageOverhang,a.y+y,imageWidth,ph);
 move(child(c,11),lv_obj_get_parent(c),a.x-imageOverhang,a.y+y,imageWidth,ph);
 lv_obj_t *player=child(c,11);plain(player);lv_obj_add_flag(player,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
 lv_obj_t *top=lv_obj_get_child(player,0);plain(top);box(top,0,0,imageWidth,LV_MAX(1,ph-32));lv_obj_add_flag(top,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
 if(lv_obj_get_child_cnt(top)>1){
  lv_obj_t *well=lv_obj_get_child(top,1);
  box(well,0,0,imageWidth,LV_MAX(1,ph-32));
  lv_obj_set_style_bg_color(well,lv_color_hex(UI_COLOR_IMAGE_SURFACE),0);
  lv_obj_set_style_bg_opa(well,LV_OPA_COVER,0);lv_obj_set_style_radius(well,12,0);
 }
 box(child(c,16),0,ph-29,imageWidth,4);
 box(lv_obj_get_parent(child(c,17)),imageOverhang,ph-23,w,23);
 lv_obj_t *idle=child(c,10);plain(idle);lv_obj_add_flag(idle,LV_OBJ_FLAG_OVERFLOW_VISIBLE);int idleHeight=h-y;box(idle,a.x-imageOverhang,a.y+y,imageWidth,idleHeight);
 for(unsigned i=0;i<lv_obj_get_child_cnt(idle);i++){
  lv_obj_t *o=lv_obj_get_child(idle,i);
  if(lv_obj_check_type(o,&lv_image_class)){lv_obj_set_style_bg_color(o,lv_color_hex(UI_COLOR_IMAGE_SURFACE),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,12,0);int side=LV_MAX(1,ph-32);lv_obj_set_size(o,imageWidth,side);lv_image_set_inner_align(o,LV_IMAGE_ALIGN_CONTAIN);lv_obj_align(o,LV_ALIGN_TOP_MID,0,0);}
  else if(lv_obj_check_type(o,&lv_label_class)){
   if(i==0){
    const lv_font_t *f=ui_text_source(lv_obj_get_style_text_font(o,0));lv_area_t ink=ui_layout_ink(f,lv_label_get_text(o));
    unsigned zoom=LV_MIN(256,LV_MIN(w*256/LV_MAX(1,lv_area_get_width(&ink)),LV_MAX(1,idleHeight-24)*256/LV_MAX(1,lv_area_get_height(&ink))));
    ui_text_scale(o,f,zoom);lv_obj_set_size(o,LV_SIZE_CONTENT,LV_SIZE_CONTENT);/* The idle logo is centred on the physical screen, independent of
     * the telemetry rows and the expanded model-image surface. */
    int screenW=lv_disp_get_hor_res(NULL),screenH=lv_disp_get_ver_res(NULL);
    ui_layout_ink_place(o,(imageWidth-screenW)/2,-(a.y+y),screenW,screenH);
   }else{lv_obj_set_width(o,w);lv_obj_align(o,LV_ALIGN_BOTTOM_MID,0,0);}
  }
 }
 lv_obj_t *controller=child(c,12);plain(controller);
 // Buttons are outside the preview; only their parent changes, not commands.
 lv_obj_t *root=lv_obj_get_parent(c);
 move(child(c,14),root,a.x+w/2-hit*3/2-g,a.y+h+g,hit,hit);
 move(child(c,15),root,a.x+w/2-hit/2,a.y+h+g,hit,hit);
 move(child(c,27),root,a.x+w/2+hit/2+g,a.y+h+g,hit,hit);
 lv_obj_add_flag(controller,LV_OBJ_FLAG_HIDDEN);
 move(child(c,23),c,0,h-hit,w,hit);
 lv_obj_set_style_bg_opa(child(c,23),LV_OPA_TRANSP,0);
 compact_text(device,18);compact_text(temps,20);compact_text(child(c,23),20);
 int dw=w,dh=hit*3/4;
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
 dw=sw*48/100;
#endif
 int mw=dw*36/100;
 lv_obj_t *model=lv_obj_get_child(device,0),*name=lv_obj_get_child(device,1);
 box(model,g,0,mw,dh);ui_layout_device_model(model);
 int nh=lv_obj_get_style_text_font(name,0)->line_height;
 box(name,g+mw+g,(dh-nh)/2,dw-mw-3*g,nh);
 lv_obj_set_style_text_align(name,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_set_style_bg_color(device,lv_color_hex(UI_COLOR_SURFACE_SUBTLE),0);
 lv_obj_set_style_bg_color(child(c,2),lv_color_hex(UI_COLOR_SURFACE_SUBTLE),0);
 ui_layout_print_button(child(c,14));ui_layout_print_button(child(c,15));
 ui_shape_home_state(c);
 lv_obj_add_event_cb(c,home_connection,LV_EVENT_MSG_RECEIVED,NULL);
 lv_msg_subsribe_obj(XTOUCH_ON_MQTT,c,NULL);
}
void ui_shape_home_state(lv_obj_t *c){
 bool active=bambuStatus.print_status==XTOUCH_PRINT_STATUS_RUNNING||bambuStatus.print_status==XTOUCH_PRINT_STATUS_PAUSED||bambuStatus.print_status==XTOUCH_PRINT_STATUS_PREPARE;
 for(unsigned i=14;i<=15;i++){if(active)lv_obj_clear_flag(child(c,i),LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(child(c,i),LV_OBJ_FLAG_HIDDEN);}
 if(xtouch_printer_online())lv_obj_clear_flag(child(c,27),LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(child(c,27),LV_OBJ_FLAG_HIDDEN);
 ui_shape_area_t a=ui_shape_area(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL),false);
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_SQUARE
 a.h-=a.hit+a.gap;
#endif
 lv_obj_set_x(child(c,27),a.x+a.w/2-a.hit/2+(active?a.hit+a.gap:0));
}
void ui_shape_text(lv_obj_t *o,unsigned pixels){
 const lv_font_t *font=ui_text_source(lv_obj_get_style_text_font(o,0));
 ui_text_scale(o,font,pixels*256/LV_MAX(1,font->line_height));
 lv_obj_add_flag(o,LV_OBJ_FLAG_USER_3); /* Explicit shape typography role. */
}
void ui_shape_typography(lv_obj_t *o){
 if(lv_obj_check_type(o,&lv_label_class)&&!lv_obj_has_flag(o,LV_OBJ_FLAG_USER_3)){
  const lv_font_t *f=ui_text_source(lv_obj_get_style_text_font(o,0));
  if(f!=&ui_font_xlcd&&f!=&ui_font_xlcdmin&&f!=&ui_font_xperiments&&f!=&ui_font_symbols_28){
   const char *t=lv_label_get_text(o);unsigned char c=t[0];
   if(!(c==0xef&&(unsigned char)t[1]<0xb0))ui_shape_text(o,strchr(t,':')?16:(c>='0'&&c<='9')?24:18);
  }
 }
 for(unsigned i=0;i<lv_obj_get_child_cnt(o);i++)ui_shape_typography(lv_obj_get_child(o,i));
}
void ui_shape_setup_choices(lv_obj_t *root,lv_obj_t *title){
 int sw=lv_disp_get_hor_res(NULL),sh=lv_disp_get_ver_res(NULL),row=0,label=0;
 ui_shape_area_t a=ui_shape_area(sw,sh,true);
 for(unsigned i=0;i<lv_obj_get_child_cnt(root);i++){
  lv_obj_t *o=lv_obj_get_child(root,i);if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN)||o==title)continue;
  if(lv_obj_check_type(o,&lv_button_class)){
   box(o,a.x,sh*30/100+row*sh*22/100,a.w,sh*18/100);row++;
   if(lv_obj_get_child_cnt(o)>1){
    release_flex(o);lv_obj_set_layout(o,0);lv_obj_set_style_pad_all(o,0,0);
    int side=sh*12/100,rh=sh*18/100,inset=a.gap*2;
    lv_obj_t *slot=lv_obj_get_child(o,0);box(slot,inset,(rh-side)/2,side,side);
    lv_obj_t *column=lv_obj_get_child(o,1);plain(column);
    int cw=a.w-side-inset*2-a.gap;box(column,inset+side+a.gap,0,cw,rh);
    compact_text(column,20);
    for(unsigned n=0;n<lv_obj_get_child_cnt(column);n++){
     lv_obj_t *text=lv_obj_get_child(column,n);box(text,0,rh/2+(n?4:-26),cw,24);
     lv_label_set_long_mode(text,LV_LABEL_LONG_SCROLL);lv_obj_set_style_text_align(text,LV_TEXT_ALIGN_LEFT,0);
    }
   }
  }else if(lv_obj_check_type(o,&lv_label_class)){
   lv_obj_set_width(o,a.w);lv_obj_set_align(o,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(o,a.x,sh*(label++?77:23)/100);
   lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(o,LV_LABEL_LONG_SCROLL);
  }
 }
 ui_shape_theme(root);lv_obj_update_layout(root);
}
void ui_shape_screen(lv_obj_t *root,int page){
 if(page<0)return;
 release_flex(root);
 lv_obj_set_layout(root,0);
 int w=lv_disp_get_hor_res(NULL),h=lv_disp_get_ver_res(NULL);
 ui_shape_area_t a=ui_shape_area(w,h,false);
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
 if(page==0){
  /* Reserve the same gap on both sides of the print controls. The bottom
   * navigation target is circular, but its full touch box must stay clear. */
  int navigation_top=h*94/100-a.hit/2;
  a.h=LV_MIN(a.h,navigation_top-a.y-a.hit-2*a.gap);
 }
#endif
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_SQUARE
 if(page==0)a.h-=a.hit+a.gap;
#endif
 lv_obj_t *component=page==0?ui_homeComponent:page==1?ui_temperatureComponent:page==2?ui_controlComponent:page==3?ui_filamentComponent:ui_settingsComponent;
 nav(root,ui_sidebarComponent,w,h);
 box(component,a.x,a.y,a.w,a.h);lv_obj_set_style_pad_all(component,0,0);
 lv_obj_set_style_border_width(component,0,0);lv_obj_set_style_radius(component,0,0);
 if(page==0){plain(component);home(component,a);}
 else if(page==1){
  plain(component);int half=(a.h-a.gap)/2;
  const unsigned groups[]={1,10};
  for(unsigned j=0;j<2;j++){
   lv_obj_t *r=child(component,groups[j]);box(r,0,j*(half+a.gap),a.w,half);
   lv_obj_set_flex_flow(r,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_all(r,0,0);lv_obj_set_style_pad_column(r,a.gap,0);
   for(unsigned k=0;k<lv_obj_get_child_cnt(r);k++){lv_obj_t *card=lv_obj_get_child(r,k);lv_obj_set_width(card,0);lv_obj_set_flex_grow(card,1);lv_obj_set_height(card,lv_pct(100));}
  }
  box(child(component,26),0,0,a.w,a.h);
 }
 else if(page==3){plain(component);}
 else if(page==2){
  plain(component);int cell=(a.w-2*a.gap)/3,rh=(a.h-2*a.gap)/3;
  const unsigned keys[]={2,9,16,5,11,18,7,13,20};
  for(unsigned i=0;i<9;i++){lv_obj_t *key=child(component,keys[i]);move(key,component,(i%3)*(cell+a.gap),(i/3)*(rh+a.gap),cell,rh);lv_obj_set_style_pad_all(key,0,0);}
  const unsigned cols[]={1,8,15};for(unsigned i=0;i<3;i++)lv_obj_add_flag(child(component,cols[i]),LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(child(component,7),LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(child(component,20),LV_OBJ_FLAG_HIDDEN);
 }
 else if(page!=4){
  // Existing interactive controls remain intact; the native-width container
  // owns overflow instead of clipping or scaling a landscape screenshot.
  lv_obj_clear_flag(component,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_layout(root,0);
  lv_obj_set_flex_flow(component,page==2?LV_FLEX_FLOW_ROW:LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(component,a.gap,0);
  lv_obj_set_scroll_dir(component,LV_DIR_VER);lv_obj_add_flag(component,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(component,LV_SCROLLBAR_MODE_AUTO);
  for(unsigned i=0;i<lv_obj_get_child_cnt(component);i++){
   lv_obj_t *o=lv_obj_get_child(component,i);
   if(page!=2)lv_obj_set_width(o,lv_pct(100));
  }
 }
 ui_shape_typography(component);
 if(page==1){
  /* Temperature/fan cards retain their existing controls, but use a vertical
   * composition instead of squeezing landscape rows into narrow columns. */
  lv_obj_update_layout(component);
  const unsigned cards[]={2,6,11,16,21};
  for(unsigned i=0;i<5;i++){
   lv_obj_t *card=child(component,cards[i]);int cw=lv_obj_get_width(card),ch=lv_obj_get_height(card);
   lv_obj_set_layout(card,0);lv_obj_set_style_pad_all(card,0,0);
   unsigned count=lv_obj_get_child_cnt(card);
   for(unsigned n=0;n<count;n++){
    lv_obj_t *item=lv_obj_get_child(card,n);lv_obj_set_style_pad_all(item,0,0);
    lv_obj_set_style_text_align(item,LV_TEXT_ALIGN_CENTER,0);
    if(i<2){compact_text(item,n==0?26:26);box(item,4,ch*(1+3*n)/10,cw-8,32);}
    else if(n==0){compact_text(item,26);box(item,4,ch/12,cw-8,32);}
    else if(n==1){compact_text(item,18);box(item,4,ch*4/10,cw-8,24);lv_label_set_long_mode(item,LV_LABEL_LONG_SCROLL);}
    else {compact_text(item,n==2?18:24);box(item,n==2?cw-24:4,ch*7/10,n==2?20:cw-30,30);}
   }
  }
 }
 if(page==4){
  lv_obj_update_layout(component);
  for(unsigned i=0;i<lv_obj_get_child_cnt(component);i++){
   lv_obj_t *row=lv_obj_get_child(component,i);
   if(lv_obj_check_type(row,&lv_label_class)&&lv_obj_has_flag(row,LV_OBJ_FLAG_CLICKABLE))ui_layout_button_text(row);
   if(lv_obj_check_type(row,&lv_button_class))ui_layout_caption_refresh(row);
  }
  ui_layout_captions_tree(component);
 }
 ui_shape_theme(root);lv_obj_set_style_bg_color(root,lv_color_hex(UI_COLOR_SCREEN),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
 /* Only the five circular targets intercept taps; their transparent parent
  * stays above cards without covering the page's interactive content. */
 lv_obj_move_foreground(ui_sidebarComponent);
#if XTOUCH_UI_LAYOUT==UI_LAYOUT_ROUND
 ring_bind(root);
#endif
 lv_obj_update_layout(root);
}
void ui_shape_settings(lv_obj_t *c,const ui_shape_setting_t *rows,unsigned count){
 release_flex(c);
 ui_shape_area_t a=ui_shape_area(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL),false);
 /* Preserve the original continuous settings order, controls and callbacks.
  * Only the viewport changes; no category cards or duplicate icons exist. */
 for(unsigned i=0;i<lv_obj_get_child_cnt(c);i++)lv_obj_add_flag(lv_obj_get_child(c,i),LV_OBJ_FLAG_HIDDEN);
 plain(c);box(c,a.x,a.y,a.w,a.h);
 lv_obj_set_flex_flow(c,LV_FLEX_FLOW_COLUMN);
 lv_obj_set_flex_align(c,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START);
 lv_obj_set_style_pad_gap(c,a.gap,0);
 lv_obj_add_flag(c,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(c,LV_DIR_VER);
 lv_obj_set_scrollbar_mode(c,LV_SCROLLBAR_MODE_AUTO);
 for(unsigned j=0;j<count;j++)if(rows[j].row){
  lv_obj_t *r=rows[j].row;
  lv_obj_move_to_index(r,-1);
  lv_obj_clear_flag(r,LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_width(r,lv_pct(100));lv_obj_set_height(r,LV_SIZE_CONTENT);
  lv_obj_set_style_max_height(r,LV_COORD_MAX,0);lv_obj_set_style_min_height(r,44,0);
  lv_obj_set_style_pad_ver(r,4,0);lv_obj_set_style_pad_left(r,12,0);lv_obj_set_style_pad_right(r,12,0);lv_obj_set_style_bg_opa(r,LV_OPA_TRANSP,0);
  lv_obj_set_style_border_width(r,1,0);lv_obj_set_style_border_side(r,LV_BORDER_SIDE_BOTTOM,0);
  lv_obj_set_style_border_color(r,lv_color_hex(UI_COLOR_BORDER),0);
  for(unsigned k=0;k<lv_obj_get_child_count(r);k++){
   lv_obj_t *control=lv_obj_get_child(r,k);
   if(lv_obj_check_type(control,&lv_dropdown_class)){
    lv_obj_set_height(control,32);lv_obj_set_style_min_height(control,0,0);lv_obj_set_style_pad_ver(control,0,0);
   }
   if(lv_obj_check_type(control,&lv_switch_class)){
    lv_obj_set_height(r,44);lv_obj_set_style_max_height(r,44,0);
    lv_obj_set_flex_flow(r,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(r,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(r,a.gap,0);
    lv_obj_clear_flag(control,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_flex_grow(control,0);
    lv_obj_set_style_min_width(control,52,0);lv_obj_set_style_max_width(control,52,0);
    lv_obj_set_style_min_height(control,26,0);lv_obj_set_style_max_height(control,26,0);lv_obj_set_size(control,52,26);
    lv_obj_set_ext_click_area(control,9); /* Compact visual, 44px-tall touch target. */
    for(unsigned n=0;n<lv_obj_get_child_cnt(r);n++){
     lv_obj_t *caption=lv_obj_get_child(r,n);if(caption==control||!lv_obj_check_type(caption,&lv_label_class))continue;
     lv_obj_clear_flag(caption,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_set_width(caption,0);lv_obj_set_flex_grow(caption,1);lv_obj_set_style_pad_all(caption,0,0);lv_label_set_long_mode(caption,LV_LABEL_LONG_SCROLL);
    }
   }
  }
  if(lv_obj_check_type(r,&lv_label_class)&&lv_obj_has_flag(r,LV_OBJ_FLAG_CLICKABLE)){
   lv_obj_set_height(r,44);lv_obj_set_style_pad_all(r,0,0);lv_obj_set_style_text_align(r,LV_TEXT_ALIGN_CENTER,0);
  }
 }
}
void ui_shape_dialog(lv_obj_t *c,bool error){
 int w=lv_disp_get_hor_res(NULL),h=lv_disp_get_ver_res(NULL);
 if(!lv_obj_get_child_cnt(c))return;
 lv_obj_t *panel=lv_obj_get_child(c,0);
 ui_shape_theme(panel);lv_obj_set_style_bg_color(panel,lv_color_hex(UI_COLOR_SURFACE),0);
 ui_shape_area_t safe=ui_shape_area(w,h,true);
 lv_obj_set_width(panel,safe.w);
 lv_obj_set_style_max_height(panel,safe.h,0);
 lv_obj_center(panel);(void)error;
}
void ui_shape_keypad(lv_obj_t *c,lv_obj_t *input,lv_obj_t **tabs,unsigned count,lv_obj_t *more){
 release_flex(c);
 ui_shape_area_t a=ui_shape_area(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL),true);
 box(c,a.x,a.y,a.w,a.h);plain(c);
 lv_obj_t *k=child(c,26);plain(k);box(k,0,0,a.w,a.h);lv_obj_move_foreground(k);
 int g=a.gap,top=0;
 if(count){
  int n=count+(more?1:0),tw=(a.w-(n-1)*g)/n;
  for(unsigned i=0;i<count;i++){move(tabs[i],k,i*(tw+g),0,tw,a.hit);if(lv_obj_get_child_cnt(tabs[i])){lv_obj_t *l=lv_obj_get_child(tabs[i],0);lv_obj_set_width(l,tw-8);lv_label_set_long_mode(l,LV_LABEL_LONG_SCROLL);lv_obj_center(l);}}
  if(more)move(more,k,count*(tw+g),0,tw,a.hit);
  top=a.hit+g;
 }
 move(input,k,0,top,a.w,a.hit);top+=a.hit+g;
 lv_obj_clear_flag(input,LV_OBJ_FLAG_HIDDEN);lv_obj_set_style_text_align(input,LV_TEXT_ALIGN_CENTER,0);
 int kw=(a.w-2*g)/3,kh=(a.h-top-3*g)/4;
 const unsigned keys[]={28,29,30,31,32,33,34,35,36,38,37,39};
 for(unsigned i=0;i<12;i++){
  lv_obj_t *key=child(c,keys[i]);move(key,k,(i%3)*(kw+g),top+(i/3)*(kh+g),kw,kh);
  lv_obj_set_style_text_align(key,LV_TEXT_ALIGN_CENTER,0);lv_obj_set_style_pad_all(key,0,0);
  lv_obj_set_style_pad_top(key,LV_MAX(0,(kh-lv_obj_get_style_text_font(key,0)->line_height)/2),0);
 }
 lv_obj_t *unused=child(c,27);if(unused!=input)lv_obj_add_flag(unused,LV_OBJ_FLAG_HIDDEN);
 ui_shape_theme(c);lv_obj_update_layout(c);
}
#else
void ui_shape_place(lv_obj_t *o,int x,int y,int w,int h){(void)o;(void)x;(void)y;(void)w;(void)h;}
void ui_shape_text(lv_obj_t *o,unsigned p){(void)o;(void)p;}
void ui_shape_typography(lv_obj_t *o){(void)o;}
void ui_shape_theme(lv_obj_t *o){(void)o;}
void ui_shape_screen(lv_obj_t *root,int page){(void)root;(void)page;}
void ui_shape_home_state(lv_obj_t *c){(void)c;}
void ui_shape_setup_choices(lv_obj_t *s,lv_obj_t *t){(void)s;(void)t;}
void ui_shape_dialog(lv_obj_t *c,bool error){(void)c;(void)error;}
void ui_shape_settings(lv_obj_t *c,const ui_shape_setting_t *r,unsigned n){(void)c;(void)r;(void)n;}
void ui_shape_keypad(lv_obj_t *c,lv_obj_t *i,lv_obj_t **t,unsigned n,lv_obj_t *m){(void)c;(void)i;(void)t;(void)n;(void)m;}
#endif
