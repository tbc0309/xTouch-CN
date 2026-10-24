/* Shared, native-pixel device picker. Only the asset resolution is board-selected.
 * Layout, drawing, scrolling and hit testing use the same measured geometry. */
#include "ui.h"
#include "printer_picker.h"
#include "printer_models.h"
#include "source_icons.h"
#include "settings_gesture.h"
#include <stdio.h>
#include <string.h>

/* This modal measures native artwork and fonts; bypass the legacy design canvas. */
#undef lv_obj_create
#undef lv_obj_set_size
#undef lv_obj_set_pos
#undef lv_obj_set_height
#undef lv_obj_get_coords
#undef lv_obj_scroll_to_y
#undef lv_obj_set_style_width
#undef lv_obj_set_style_radius
#undef lv_indev_get_point
#undef lv_txt_get_size
#undef lv_draw_rect
#undef lv_draw_img
#undef lv_draw_label

static lv_obj_t *overlay,*viewport,*content,*covered_screen;
static ui_scroll_guard_t *selection_guard;
static uint8_t slots[XTOUCH_MAX_PRINTERS];
static unsigned count;
static bool closing,names_overflow;
static uint32_t name_epoch;
static lv_timer_t *name_timer;
static struct {
    lv_area_t panel,close,toggle;
    int unit,width,height,row,columns,cell,art_width,art_height,title_height,name_height;
} layout;
static void fill_viewport(void);
/* Use the regular body tier for both the model caption and device name. */
static const lv_img_dsc_t *source_icon(uint8_t source);
static const lv_font_t *picker_font(void){return LV_FONT_DEFAULT;}
static struct {lv_area_t ink;int width,height;} source_metrics[4];
static void measure_sources(void){
    for(unsigned i=0;i<4;i++){
        const lv_img_dsc_t *image=source_icon(i);lv_area_t ink={image->header.w,image->header.h,-1,-1};
        for(int y=0;y<image->header.h;y++)for(int x=0;x<image->header.w;x++)if(image->data[y*image->header.w+x]>=32){
            ink.x1=LV_MIN(ink.x1,x);ink.x2=LV_MAX(ink.x2,x);ink.y1=LV_MIN(ink.y1,y);ink.y2=LV_MAX(ink.y2,y);
        }
        source_metrics[i].ink=ink;
        source_metrics[i].width=lv_area_get_width(&ink);
        source_metrics[i].height=lv_area_get_height(&ink);
    }
}
static bool image_mode(void){return XTOUCH_UI_PICTURE_PICKER&&!xTouchConfig.xTouchPrinterPickerList;}
static const lv_img_dsc_t *source_icon(uint8_t source){
    return source==0?&source_img_0:source==1?&source_img_1:source==3?&source_img_3:&source_img_2;
}
static void measure(void){
    int w=lv_disp_get_hor_res(NULL),h=lv_disp_get_ver_res(NULL);
    int u=LV_MAX(1,LV_MIN(w/320,h/240));layout.unit=u;
    layout.panel=(lv_area_t){0,0,w-1,h-1};
    layout.close=(lv_area_t){w-36*u,6*u,w-6*u-1,36*u-1};
    layout.toggle=layout.close;layout.toggle.x1-=36*u;layout.toggle.x2-=36*u;
    layout.width=lv_area_get_width(&layout.panel)-20*u;
    layout.height=lv_area_get_height(&layout.panel)-48*u;
    layout.title_height=picker_font()->line_height;
    layout.name_height=layout.title_height;
    measure_sources();
    layout.art_width=layout.art_height=0;
    // Keep unknown models in the same grid as known models.
    lv_img_header_t reference;
    if(image_mode()&&lv_img_decoder_get_info(ui_printer_model_image("C12"),&reference)==LV_RES_OK){
        layout.art_width=reference.w;layout.art_height=reference.h;
    }
    // Reserve for the largest asset, so mixed models never change the grid.
    if(image_mode())for(unsigned i=0;i<count;++i){
        const lv_img_dsc_t *model=ui_printer_model_image(xtouch_printer_model_at(slots[i]));
        lv_img_header_t info;
        if(model&&lv_img_decoder_get_info(model,&info)==LV_RES_OK){
            layout.art_width=LV_MAX(layout.art_width,info.w);
            layout.art_height=LV_MAX(layout.art_height,info.h);
        }
    }
    layout.columns=image_mode()?LV_CLAMP(1,layout.width/LV_MAX(1,layout.art_width+8*u),3):1;
    layout.cell=layout.width/layout.columns;
    layout.row=image_mode()?layout.art_height+layout.title_height+layout.name_height+20*u:40*u;
    if(image_mode()){
        int visible_rows=LV_MAX(1,layout.height/layout.row);
        layout.row=LV_MAX(layout.row,layout.height/visible_rows);
    }else layout.height=LV_MIN(layout.height,LV_MAX(1,count)*layout.row);
}
static void text_font(lv_draw_ctx_t *ctx,lv_area_t area,const char *value,uint32_t color,bool center,const lv_font_t *font){
    lv_area_t clip;if(!_lv_area_intersect(&clip,&area,ctx->clip_area))return;
    lv_area_t ink=ui_layout_ink(font,value);
    lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=font;d.color=lv_color_hex(color);
    d.flag=LV_TEXT_FLAG_EXPAND;
    area.x1+=(center?(lv_area_get_width(&area)-(ink.x2-ink.x1+1))/2:0)-ink.x1;
    area.y1+=(lv_area_get_height(&area)-(ink.y2-ink.y1+1))/2-ink.y1;
    const lv_area_t *previous=ctx->clip_area;ctx->clip_area=&clip;
    lv_draw_label(ctx,&d,&area,value,NULL);ctx->clip_area=previous;
}
static void text(lv_draw_ctx_t *ctx,lv_area_t area,const char *value,uint32_t color,bool center){
    text_font(ctx,area,value,color,center,picker_font());
}
static bool contains(lv_area_t a,lv_point_t p){return p.x>=a.x1&&p.x<=a.x2&&p.y>=a.y1&&p.y<=a.y2;}
static int name_offset(int overflow){
    if(overflow<=0||!XTOUCH_UI_PICTURE_PICKER)return 0;
    unsigned travel=LV_MAX(1,overflow*1000/(12*layout.unit));
    unsigned phase=lv_tick_elaps(name_epoch)%(travel*2+2000);
    if(phase<1000)return 0;
    phase-=1000;
    if(phase<travel)return phase*overflow/travel;
    phase-=travel;
    if(phase<1000)return overflow;
    return overflow-(phase-1000)*overflow/travel;
}
static void draw_name(lv_draw_ctx_t *ctx,lv_area_t area,const char *name,uint8_t source,uint32_t color){
    const lv_img_dsc_t *badge=source_icon(source);
    lv_area_t ink=ui_layout_ink(picker_font(),name);
    unsigned index=source<4?source:2;
    int icon_width=source_metrics[index].width,icon_height=source_metrics[index].height;
    // Reserve one shared square cell, independent of the source silhouette.
    // This keeps names aligned when Wi-Fi, cloud, phone and mail badges differ.
    int icon_cell=source_img_0.header.w;
    int text_width=ink.x2-ink.x1+1,gap=3*layout.unit;
    int available=lv_area_get_width(&area),group=LV_MIN(available,text_width+icon_cell+gap);
    int start=area.x1+(image_mode()?(available-group)/2:0);
    int x=start+(icon_cell-icon_width)/2-source_metrics[index].ink.x1;
    int y=area.y1+(lv_area_get_height(&area)-icon_height)/2-source_metrics[index].ink.y1;
    lv_area_t icon_box={x,y,x+badge->header.w-1,y+badge->header.h-1},clip;
    lv_draw_img_dsc_t icon;lv_draw_img_dsc_init(&icon);icon.recolor=lv_color_hex(color);icon.recolor_opa=LV_OPA_COVER;
    // Align visible alpha bounds with the text ink, rather than the padded image canvas.
    if(_lv_area_intersect(&clip,&area,ctx->clip_area)){
        const lv_area_t *previous=ctx->clip_area;ctx->clip_area=&clip;
        lv_draw_img(ctx,&icon,&icon_box,badge);ctx->clip_area=previous;
    }
    lv_area_t window={start+icon_cell+gap,area.y1,start+group-1,area.y2};
    if(window.x1>window.x2||!_lv_area_intersect(&clip,&window,ctx->clip_area))return;
    int overflow=text_width-lv_area_get_width(&window);
    if(overflow>0)names_overflow=true;
    const lv_area_t *previous=ctx->clip_area;ctx->clip_area=&clip;
    window.x1-=name_offset(overflow);window.x2=LV_MAX(window.x2,window.x1+text_width-1);
    text(ctx,window,name,0xFFFFFF,false);ctx->clip_area=previous;
}
static lv_area_t centered_box(lv_area_t button,int width,int height){
    int x=button.x1+(lv_area_get_width(&button)-width)/2;
    int y=button.y1+(lv_area_get_height(&button)-height)/2;
    return (lv_area_t){x,y,x+width-1,y+height-1};
}
static void draw(lv_event_t *e){
    lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(e);int u=layout.unit;
    lv_draw_rect_dsc_t r;lv_draw_rect_dsc_init(&r);r.bg_opa=LV_OPA_COVER;r.radius=0;r.bg_color=lv_color_hex(0x444444);
    lv_draw_rect(ctx,&r,&layout.panel);
    int title_end=(XTOUCH_UI_PICTURE_PICKER?layout.toggle.x1:layout.close.x1)-6*u;
    text(ctx,(lv_area_t){layout.panel.x1+14*u,layout.panel.y1+6*u,title_end,layout.panel.y1+35*u},XTOUCH_TEXT_SELECT_DEVICE,0xFFFFFF,false);
    if(XTOUCH_UI_PICTURE_PICKER){
        lv_area_t a=layout.toggle;r.radius=6*u;r.bg_color=lv_color_hex(0x555555);lv_draw_rect(ctx,&r,&a);
        r.radius=0;r.bg_color=lv_color_white();
        lv_area_t symbol=centered_box(a,18*u,(image_mode()?14:18)*u);
        // List bars carry more ink than bullets; balance their optical center.
        int left=symbol.x1-(image_mode()?u:0),top=symbol.y1;
        if(image_mode())for(int i=0;i<3;++i){
            lv_area_t dot={left,top+i*6*u,left+2*u-1,top+(i*6+2)*u-1};
            lv_area_t line={left+5*u,dot.y1,left+18*u-1,dot.y2};lv_draw_rect(ctx,&r,&dot);lv_draw_rect(ctx,&r,&line);
        }else for(int y=0;y<2;++y)for(int x=0;x<2;++x){
            lv_area_t tile={left+x*11*u,top+y*11*u,left+(x*11+7)*u-1,top+(y*11+7)*u-1};lv_draw_rect(ctx,&r,&tile);
        }
    }
    r.radius=6*u;r.bg_color=lv_color_hex(0xAA2A00);lv_draw_rect(ctx,&r,&layout.close);
    lv_area_t circle=centered_box(layout.close,18*u,18*u);
    r.radius=LV_RADIUS_CIRCLE;r.bg_color=lv_color_white();lv_draw_rect(ctx,&r,&circle);
    lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.color=lv_color_hex(0xAA2A00);line.width=2*u;line.round_start=line.round_end=1;
    lv_point_t a={circle.x1+5*u,circle.y1+5*u},b={circle.x2-5*u,circle.y2-5*u};
    lv_draw_line(ctx,&line,&a,&b);a.y=circle.y2-5*u;b.y=circle.y1+5*u;lv_draw_line(ctx,&line,&a,&b);
}

static void draw_devices(lv_event_t *e){
    lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(e);lv_area_t a;lv_obj_get_coords(content,&a);int u=layout.unit;
    unsigned first=ctx->clip_area->y1>a.y1?(ctx->clip_area->y1-a.y1)/layout.row*layout.columns:0;
    for(unsigned i=first;i<count;++i){
        int x=a.x1+(i%layout.columns)*layout.cell,y=a.y1+(i/layout.columns)*layout.row;
        if(y>ctx->clip_area->y2)break;
        lv_area_t card={x,y,x+layout.cell-(image_mode()?4*u:1)-1,y+layout.row-(image_mode()?8:6)*u-1};
        bool active=slots[i]==xtouch_printer_active();
        lv_draw_rect_dsc_t r;lv_draw_rect_dsc_init(&r);r.bg_opa=LV_OPA_COVER;r.radius=5*u;
        r.bg_color=lv_color_hex(0x555555);r.border_width=u;r.border_opa=LV_OPA_COVER;r.border_color=lv_color_hex(active?0x2AFF00:0x777777);
        lv_draw_rect(ctx,&r,&card);
        char formatted[100];const char *name=xtouch_printer_name_at(slots[i]);
        if(image_mode()){
            int title_y=y+4*u;
            text(ctx,(lv_area_t){card.x1+4*u,title_y,card.x2-4*u,title_y+layout.title_height-1},ui_printer_model_short_name(xtouch_printer_model_at(slots[i])),0xFFFFFF,true);
            const lv_img_dsc_t *model=ui_printer_model_image(xtouch_printer_model_at(slots[i]));lv_img_header_t h;
            if(model&&lv_img_decoder_get_info(model,&h)==LV_RES_OK){
                int left=(card.x1+card.x2+1-h.w)/2,top=title_y+layout.title_height+2*u;
                lv_area_t bounds={left,top,left+h.w-1,top+h.h-1},clip;
                if(_lv_area_intersect(&clip,&card,ctx->clip_area)){
                    const lv_area_t *previous=ctx->clip_area;ctx->clip_area=&clip;
                    lv_draw_img_dsc_t d;lv_draw_img_dsc_init(&d);lv_draw_img(ctx,&d,&bounds,model);ctx->clip_area=previous;
                }
            }else{
                int top=title_y+layout.title_height+2*u;
                text(ctx,(lv_area_t){card.x1+4*u,top,card.x2-4*u,top+layout.art_height-1},"xTouch-CN",0xCCCCCC,true);
            }
        }else{
            snprintf(formatted,sizeof(formatted),"[%s] %s",ui_printer_model_short_name(xtouch_printer_model_at(slots[i])),name);name=formatted;
            lv_area_t dot={card.x2-19*u,y+13*u,card.x2-12*u,y+20*u};r.radius=LV_RADIUS_CIRCLE;r.bg_color=lv_color_hex(active?0x2AFF00:0xAAAAAA);r.border_width=0;lv_draw_rect(ctx,&r,&dot);
        }
        int top=image_mode()?y+4*u+layout.title_height+2*u+layout.art_height+3*u:y+(layout.row-6*u-layout.name_height)/2;
        draw_name(ctx,(lv_area_t){card.x1+4*u,top,card.x2-(image_mode()?4:28)*u,top+layout.name_height-1},name,xtouch_printer_source(slots[i]),xtouch_printer_source_color(slots[i]));
    }
}
void ui_printer_picker_close(void){if(overlay&&!closing){closing=true;lv_obj_del_async(overlay);}}
static void covered_deleted(lv_event_t *e){
    if(lv_event_get_target(e)==covered_screen)covered_screen=NULL;
}
static void deleted(lv_event_t *e){
    if(lv_event_get_target(e)!=overlay)return;
    if(name_timer){lv_timer_del(name_timer);name_timer=NULL;}
    ui_printer_image_cache_end();
    if(covered_screen){
        lv_obj_remove_event_cb(covered_screen,covered_deleted);
        lv_obj_clear_flag(covered_screen,LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(covered_screen);covered_screen=NULL;
    }
    overlay=viewport=content=NULL;selection_guard=NULL;closing=false;
}
static void click(lv_event_t *e){
    (void)e;lv_indev_t *input=lv_indev_get_act();if(!input||closing)return;lv_point_t p;lv_indev_get_point(input,&p);
    if(XTOUCH_UI_PICTURE_PICKER&&contains(layout.toggle,p)){
        xTouchConfig.xTouchPrinterPickerList=!xTouchConfig.xTouchPrinterPickerList;lv_msg_send(XTOUCH_SETTINGS_SAVE,NULL);
        fill_viewport();lv_obj_invalidate(overlay);return;
    }
    if(!contains(layout.panel,p)||contains(layout.close,p))ui_printer_picker_close();
}
static void select_device(lv_event_t *e){
    (void)e;lv_indev_t *input=lv_indev_get_act();if(!input||closing)return;lv_point_t p;lv_indev_get_point(input,&p);lv_area_t a;lv_obj_get_coords(content,&a);
    if(!contains(a,p))return;
    unsigned col=(p.x-a.x1)/layout.cell,row=(p.y-a.y1)/layout.row;
    if(col>=(unsigned)layout.columns||(p.x-a.x1)%layout.cell>=layout.cell-(image_mode()?4*layout.unit:1)||(p.y-a.y1)%layout.row>=layout.row-(image_mode()?8:6)*layout.unit)return;
    unsigned index=row*layout.columns+col;
    if(index<count){xtouch_printer_request(slots[index]);ui_printer_picker_close();}
}
static void scrolled(lv_event_t *e){(void)e;name_epoch=lv_tick_get();names_overflow=false;}
static void names_tick(lv_timer_t *timer){
    (void)timer;if(!viewport||!content||closing)return;
    if(lv_obj_is_scrolling(viewport)){name_epoch=lv_tick_get();return;}
    if(!names_overflow||lv_tick_elaps(name_epoch)<1000)return;
    lv_area_t a,view;lv_obj_get_coords(content,&a);lv_obj_get_coords(viewport,&view);
    unsigned first=view.y1>a.y1?(view.y1-a.y1)/layout.row:0;
    for(unsigned row=first;row<(count+layout.columns-1)/layout.columns;++row){
        int y=a.y1+row*layout.row;if(y>view.y2)break;
        int top=image_mode()?y+9*layout.unit+layout.title_height+layout.art_height:y+(layout.row-6*layout.unit-layout.name_height)/2;
        lv_area_t footer={a.x1,top,a.x2,top+layout.name_height-1};lv_obj_invalidate_area(content,&footer);
    }
}
static void fill_viewport(void){
    ui_printer_image_cache_end();
#ifdef XTOUCH_WAVESHARE
    if(image_mode())ui_printer_image_cache_begin(lv_disp_get_hor_res(NULL)>=480?1024U*1024U:256U*1024U);
#endif
    measure();lv_obj_set_height(viewport,layout.height);lv_obj_clean(viewport);
    name_epoch=lv_tick_get();names_overflow=false;
    int total=(count+layout.columns-1)/layout.columns*layout.row;
    lv_obj_set_scrollbar_mode(viewport,total>layout.height?LV_SCROLLBAR_MODE_ON:LV_SCROLLBAR_MODE_OFF);
    content=lv_obj_create(viewport);lv_obj_remove_style_all(content);lv_obj_set_size(content,layout.width,LV_MAX(1,total));
    lv_obj_clear_flag(content,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(content,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_event_cb(content,draw_devices,LV_EVENT_DRAW_MAIN_END,NULL);lv_obj_add_event_cb(content,select_device,LV_EVENT_SHORT_CLICKED,NULL);
    ui_scroll_guard_add(selection_guard,content);
    for(unsigned i=0;i<count;++i)if(slots[i]==xtouch_printer_active()){lv_obj_update_layout(viewport);lv_obj_scroll_to_y(viewport,i/layout.columns*layout.row,LV_ANIM_OFF);break;}
}
void ui_printer_picker_show(void){
    if(overlay)return;
    closing=false;count=0;
    covered_screen=lv_scr_act();
    lv_obj_add_event_cb(covered_screen,covered_deleted,LV_EVENT_DELETE,NULL);
    lv_obj_add_flag(covered_screen,LV_OBJ_FLAG_HIDDEN);
    for(unsigned i=0;i<XTOUCH_MAX_PRINTERS;++i)if(xtouch_printer_configured(i))slots[count++]=i;
    measure();overlay=lv_obj_create(lv_layer_top());lv_obj_remove_style_all(overlay);lv_obj_set_size(overlay,lv_pct(100),lv_pct(100));
    lv_obj_set_style_bg_color(overlay,lv_color_black(),0);lv_obj_set_style_bg_opa(overlay,LV_OPA_COVER,0);lv_obj_clear_flag(overlay,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(overlay,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay,draw,LV_EVENT_DRAW_MAIN_END,NULL);lv_obj_add_event_cb(overlay,click,LV_EVENT_CLICKED,NULL);lv_obj_add_event_cb(overlay,deleted,LV_EVENT_DELETE,NULL);
    viewport=lv_obj_create(overlay);lv_obj_remove_style_all(viewport);lv_obj_set_pos(viewport,layout.panel.x1+8*layout.unit,layout.panel.y1+40*layout.unit);lv_obj_set_size(viewport,layout.width+4*layout.unit,layout.height);
    lv_obj_set_scroll_dir(viewport,LV_DIR_VER);lv_obj_clear_flag(viewport,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_width(viewport,3*layout.unit,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(viewport,lv_color_hex(0xAAAAAA),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(viewport,LV_OPA_COVER,LV_PART_SCROLLBAR);lv_obj_set_style_radius(viewport,LV_RADIUS_CIRCLE,LV_PART_SCROLLBAR);
    selection_guard=ui_scroll_guard_create(viewport);
    lv_obj_add_event_cb(viewport,scrolled,LV_EVENT_SCROLL,NULL);lv_obj_add_event_cb(viewport,scrolled,LV_EVENT_SCROLL_BEGIN,NULL);lv_obj_add_event_cb(viewport,scrolled,LV_EVENT_SCROLL_END,NULL);
    if(XTOUCH_UI_PICTURE_PICKER)name_timer=lv_timer_create(names_tick,50,NULL);
    fill_viewport();
}
