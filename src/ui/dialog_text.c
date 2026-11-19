#include "dialog_text.h"
#include "text_fit.h"
#include "palette.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Virtualize explicit lines: only a screenful of labels is allocated, even
   for a long HMS list. Each row owns its own horizontal marquee; short
   headings/codes therefore stay put while another row scrolls. */
static void fixed_lines_draw(lv_event_t *event){
 lv_obj_t *caption=lv_event_get_target_obj(event);const char *source=lv_label_get_text(caption);size_t size=strlen(source);
 char *fixed=malloc(size+1),*line=malloc(size+1);if(!fixed||!line){free(fixed);free(line);return;}
 const lv_font_t *font=lv_obj_get_style_text_font(caption,LV_PART_MAIN);const char *at=source;size_t used=0;
 while(*at){
  const char *end=strchr(at,'\n');size_t length=end?(size_t)(end-at):strlen(at);memcpy(line,at,length);line[length]=0;lv_point_t extent;
  lv_text_get_size(&extent,line,font,lv_obj_get_style_text_letter_space(caption,LV_PART_MAIN),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND|(lv_label_get_recolor(caption)?LV_TEXT_FLAG_RECOLOR:0));
  if(extent.x<=lv_obj_get_content_width(caption)){memcpy(fixed+used,line,length);used+=length;}
  if(!end)break;fixed[used++]='\n';at=end+1;
 }
 fixed[used]=0;lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=font;d.color=lv_obj_get_style_text_color(caption,LV_PART_MAIN);d.opa=LV_OPA_COVER;
 d.line_space=lv_obj_get_style_text_line_space(caption,LV_PART_MAIN);d.letter_space=lv_obj_get_style_text_letter_space(caption,LV_PART_MAIN);
 d.flag=LV_TEXT_FLAG_EXPAND|(lv_label_get_recolor(caption)?LV_TEXT_FLAG_RECOLOR:0);d.text=fixed;d.text_local=1;
 lv_area_t area;lv_obj_get_content_coords(caption,&area);lv_draw_label(lv_event_get_layer(event),&d,&area);free(fixed);free(line);
}
static void render_lines(lv_obj_t *caption){
 const lv_font_t *font=lv_obj_get_style_text_font(caption,LV_PART_MAIN);
 int spacing=lv_obj_get_style_text_line_space(caption,LV_PART_MAIN);
 int step=font->line_height+spacing;if(step<1)step=1;
 lv_area_t view,document;lv_obj_get_content_coords(lv_obj_get_parent(caption),&view);lv_obj_get_content_coords(caption,&document);
 unsigned first=view.y1>document.y1?(unsigned)(view.y1-document.y1)/step:0;
 unsigned capacity=lv_display_get_vertical_resolution(lv_display_get_default())/step+2;if(capacity>32)capacity=32;
 const char *text=lv_label_get_text(caption),*at=text;unsigned count=1;
 for(const char *p=text;*p;++p)if(*p=='\n')++count;
 if(first>=count)first=count-1;
 for(unsigned n=0;n<first;++n){const char *next=strchr(at,'\n');if(!next)break;at=next+1;}
 unsigned visible=count-first;if(visible>capacity)visible=capacity;
 lv_obj_t *rows[32];unsigned used=0;
 for(unsigned i=0;i<lv_obj_get_child_count(caption);++i){lv_obj_t *child=lv_obj_get_child(caption,i);if(lv_obj_has_flag(child,LV_OBJ_FLAG_USER_4)&&used<32)rows[used++]=child;}
 lv_obj_t *surface=lv_obj_get_parent(caption);
 while(lv_obj_get_parent(surface)&&lv_obj_get_style_bg_opa(surface,LV_PART_MAIN)<LV_OPA_COVER)surface=lv_obj_get_parent(surface);
 lv_color_t background=lv_obj_get_style_bg_color(surface,LV_PART_MAIN);unsigned needed=0;
 for(unsigned n=0;n<visible;++n){
  const char *end=strchr(at,'\n');size_t length=end?(size_t)(end-at):strlen(at);
  char *line=lv_malloc(length+1);if(!line)break;
  memcpy(line,at,length);line[length]=0;lv_point_t extent;
  lv_text_get_size(&extent,line,font,lv_obj_get_style_text_letter_space(caption,LV_PART_MAIN),0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND|(lv_label_get_recolor(caption)?LV_TEXT_FLAG_RECOLOR:0));
  if(extent.x>lv_obj_get_content_width(caption)){
   if(needed==used){rows[used]=lv_label_create(caption);lv_obj_add_flag(rows[used],LV_OBJ_FLAG_USER_4|LV_OBJ_FLAG_IGNORE_LAYOUT);++used;}
   lv_obj_t *row=rows[needed++];if(strcmp(lv_label_get_text(row),line))lv_label_set_text(row,line);
   lv_obj_set_style_text_font(row,font,0);lv_obj_set_style_text_opa(row,LV_OPA_COVER,0);
   lv_obj_set_style_text_color(row,lv_obj_get_style_text_color(caption,LV_PART_MAIN),0);
   lv_obj_set_style_text_letter_space(row,lv_obj_get_style_text_letter_space(caption,LV_PART_MAIN),0);
   lv_obj_set_style_text_align(row,LV_TEXT_ALIGN_LEFT,0);lv_obj_set_style_pad_all(row,0,0);
   /* Long rows are separate; fixed_lines_draw omits them from the base task. */
   lv_obj_set_style_bg_color(row,background,0);lv_obj_set_style_bg_opa(row,LV_OPA_TRANSP,0);
   lv_obj_set_size(row,lv_obj_get_content_width(caption),font->line_height);
   lv_obj_set_pos(row,0,(first+n)*step);
   lv_label_set_recolor(row,lv_label_get_recolor(caption));
   if(lv_label_get_long_mode(row)!=LV_LABEL_LONG_SCROLL)lv_label_set_long_mode(row,LV_LABEL_LONG_SCROLL);
  }
  lv_free(line);if(end)at=end+1;
 }
 while(used>needed)lv_obj_delete(rows[--used]);
}
static void body_scrolled(lv_event_t *event){render_lines((lv_obj_t *)lv_event_get_user_data(event));}
void ui_dialog_text_layout(lv_obj_t *caption,int width){
 char marker[12];snprintf(marker,sizeof(marker),"#%06X ",(unsigned)UI_COLOR_WARNING);
 bool recolor=strstr(lv_label_get_text(caption),marker)!=NULL;lv_label_set_recolor(caption,recolor);
 const lv_font_t *font=lv_obj_get_style_text_font(caption,LV_PART_MAIN);lv_point_t extent;
 lv_text_get_size(&extent,lv_label_get_text(caption),font,lv_obj_get_style_text_letter_space(caption,LV_PART_MAIN),lv_obj_get_style_text_line_space(caption,LV_PART_MAIN),LV_COORD_MAX,LV_TEXT_FLAG_EXPAND|(recolor?LV_TEXT_FLAG_RECOLOR:0));
 lv_obj_set_width(caption,width);lv_obj_set_height(caption,extent.y+lv_obj_get_style_pad_top(caption,LV_PART_MAIN)+lv_obj_get_style_pad_bottom(caption,LV_PART_MAIN));
 lv_label_set_long_mode(caption,LV_LABEL_LONG_CLIP);lv_obj_set_style_text_opa(caption,LV_OPA_TRANSP,0);
 lv_obj_remove_event_cb(caption,fixed_lines_draw);lv_obj_add_event_cb(caption,fixed_lines_draw,LV_EVENT_DRAW_MAIN,0);
 lv_obj_t *body=lv_obj_get_parent(caption);lv_obj_remove_event_cb(body,body_scrolled);lv_obj_add_event_cb(body,body_scrolled,LV_EVENT_SCROLL,caption);
 lv_obj_update_layout(caption);render_lines(caption);
}
