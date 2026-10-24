#pragma once
#include <stdio.h>
#include "layout_shared.h"
#include "i18n/language.h"
/* Semantic headings use vector badges, so no emoji/font glyph is required. */
typedef enum { UI_MESSAGE_INFO, UI_MESSAGE_WARNING, UI_MESSAGE_ERROR, UI_MESSAGE_CONFIRM } ui_message_kind;
static inline uint32_t ui_message_color(ui_message_kind kind){
 static const uint32_t colors[]={UI_COLOR_BLUE,UI_COLOR_WARNING,UI_COLOR_ERROR,UI_COLOR_BLUE};return colors[kind];
}
static inline void ui_message_caption(lv_obj_t *label,ui_message_kind kind){
 static const char *titles[8][4]={
  {"提示","警告","错误","确认操作"},{"提示","警告","錯誤","確認操作"},
  {"Info","Warning","Error","Confirm"},{"Info","Attention","Erreur","Confirmer"},
  {"Info","Warnung","Fehler","Bestätigen"},{"Информация","Внимание","Ошибка","Подтвердить"},
  {"お知らせ","警告","エラー","確認"},{"안내","주의","오류","확인"}};
 unsigned lang=xtouch_language_get();if(lang>=8)lang=2;
 lv_obj_clean(label);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
 int size=lv_obj_get_style_text_font(label,0)->line_height;
 const lv_font_t *font=lv_obj_get_style_text_font(label,0);
 int badgeSize=size*3/4,gap=size/4;
 if(gap<3)gap=3;
 /* Measure the active font so the badge gap follows its actual glyph bounds. */
 char heading[128],prefix[48]={0};unsigned spaces=0;
 lv_area_t titleInk=ui_layout_ink(font,titles[lang][kind]);
 lv_point_t advance;
 do{
  prefix[spaces++]=' ';prefix[spaces]=0;
  lv_txt_get_size(&advance,prefix,font,lv_obj_get_style_text_letter_space(label,0),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
 }while(spaces<sizeof(prefix)-1&&advance.x+titleInk.x1<badgeSize+gap);
 snprintf(heading,sizeof(heading),"%s%s:\n",prefix,titles[lang][kind]);
 lv_label_ins_text(label,0,heading);
 lv_obj_t *badge=lv_obj_create(label);lv_obj_remove_style_all(badge);
 lv_obj_set_size(badge,badgeSize,badgeSize);lv_obj_set_style_radius(badge,LV_RADIUS_CIRCLE,0);
 lv_obj_set_style_bg_color(badge,lv_color_hex(ui_message_color(kind)),0);lv_obj_set_style_bg_opa(badge,LV_OPA_COVER,0);
 lv_obj_clear_flag(badge,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(badge,LV_OBJ_FLAG_IGNORE_LAYOUT);
 lv_obj_set_pos(badge,0,size/8);
 lv_obj_t *symbol=lv_label_create(badge);lv_label_set_text(symbol,kind==UI_MESSAGE_INFO?"i":kind==UI_MESSAGE_CONFIRM?"?":"!");
 lv_obj_set_style_text_color(symbol,lv_color_white(),0);ui_text_scale(symbol,ui_text_source(lv_obj_get_style_text_font(label,0)),170*ui_layout_base_zoom()/256);
 ui_layout_ink_place(symbol,0,0,size*3/4,size*3/4);
}

/* Shared action roles: blue primary, green completion, gray dismissal, red destructive. */
typedef enum { UI_ACTION_PRIMARY, UI_ACTION_COMPLETE, UI_ACTION_SECONDARY, UI_ACTION_DESTRUCTIVE } ui_action_role;
static inline void ui_action_style(lv_obj_t *button,ui_action_role role){
 static const uint32_t normal[]={UI_COLOR_BLUE,UI_COLOR_ACTION_POSITIVE,UI_COLOR_BUTTON_SECONDARY,UI_COLOR_ACTION_DANGER};
 static const uint32_t pressed[]={UI_COLOR_BLUE_PRESSED,UI_COLOR_ACTION_POSITIVE_PRESSED,UI_COLOR_BUTTON_PRESSED,UI_COLOR_ACTION_DANGER_PRESSED};
 lv_obj_set_style_bg_color(button,lv_color_hex(normal[role]),LV_PART_MAIN|LV_STATE_DEFAULT);
 lv_obj_set_style_bg_color(button,lv_color_hex(pressed[role]),LV_PART_MAIN|LV_STATE_PRESSED);
 lv_obj_set_style_text_color(button,lv_color_white(),LV_PART_MAIN|LV_STATE_DEFAULT);
}
