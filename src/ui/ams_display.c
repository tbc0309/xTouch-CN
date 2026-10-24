#include "ams_display.h"
#include "filament_actions.h"
#include "xtouch/ams_units.h"
#include "ams_details.h"
#include <stdio.h>

unsigned ui_ams_base(void)
{
    int id = bambuStatus.m_ams_id;
    if (id < 0 || id >= XTOUCH_MAX_AMS_UNITS || !(bambuStatus.ams_exist_bits & (1ULL << id))) {
        id = 0;
        for (int i = 0; i < XTOUCH_MAX_AMS_UNITS; ++i) if (bambuStatus.ams_exist_bits & (1ULL << i)) { id = i; break; }
    }
    return (unsigned)id * 4;
}

uint32_t ui_ams_slot_color(unsigned slot)
{
    return slot < XTOUCH_MAX_AMS_SLOTS && bambuStatus.ams && bambuStatus.ams_tray_color_valid[slot]
        ? bambuStatus.ams_tray_color[slot] : 0x777777;
}

uint32_t ui_ams_remaining_color(unsigned percent)
{
    if (percent <= 10) return 0xFF6666;
    if (percent <= 20) return 0xFFD166;
    return 0xFFFFFF;
}

static uint32_t slot_border(unsigned slot)
{
    if (bambuStatus.ams && (bambuStatus.tray_reading_bits & (1ULL << slot))) return 0xFFB300;
    if (bambuStatus.ams && ui_filament_is_loaded((int)slot)) return 0x74D34A;
    return 0xAAAAAA; // Visible outline also makes black and white filament distinguishable.
}

/* Each home-page instance owns its browsing position; recreating it resets to A. */
static unsigned strip_unit(lv_obj_t *strip)
{
    unsigned unit=(unsigned)(uintptr_t)lv_obj_get_user_data(strip);
    if(unit<XTOUCH_MAX_AMS_UNITS && (bambuStatus.ams_exist_bits&(1UL<<unit)))return unit;
    for(unsigned i=0;i<XTOUCH_MAX_AMS_UNITS;++i)if(bambuStatus.ams_exist_bits&(1UL<<i))return i;
    return 0;
}
static void refresh_strip(lv_obj_t *strip)
{
    unsigned unit=strip_unit(strip),base=unit*4;
    lv_obj_set_user_data(strip,(void *)(uintptr_t)unit);
    for(unsigned i=0;i<4;++i){
        lv_obj_t *o=lv_obj_get_child(strip,i+1);
        if(i>=xtouch_ams_slot_count(unit)){lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);continue;}else lv_obj_clear_flag(o,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(o,lv_color_hex(ui_ams_slot_color(base+i)),0);
        lv_obj_set_style_border_color(o,lv_color_hex(slot_border(base+i)),0);
        lv_obj_set_style_border_width(o,bambuStatus.ams && (ui_filament_is_loaded((int)(base+i)) || (bambuStatus.tray_reading_bits&(1ULL<<(base+i))))?2:1,0);
    }
    char name[8];xtouch_ams_unit_label(unit,name,sizeof(name));lv_label_set_text(lv_obj_get_child(strip,0),name);
    unsigned units=0;for(unsigned i=0;i<XTOUCH_MAX_AMS_UNITS;++i)units+=(bambuStatus.ams_exist_bits>>i)&1U;
    bool numbered=units>1;
    if(numbered)lv_obj_clear_flag(lv_obj_get_child(strip,0),LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(lv_obj_get_child(strip,0),LV_OBJ_FLAG_HIDDEN);
    bool external=bambuStatus.external_present;
    bool dualExternal=bambuStatus.nozzle_count==2&&bambuStatus.external_multi_protocol&&(bambuStatus.external_materials&3)==3;
    for(unsigned i=5;i<7;++i){
        lv_obj_t *o=lv_obj_get_child(strip,i);
        if(external)lv_obj_clear_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_bg_color(lv_obj_get_child(strip,6),lv_color_hex(bambuStatus.external_multi_protocol&&bambuStatus.external_materials==2?bambuStatus.external_color2:bambuStatus.external_color),0);
    if(dualExternal)lv_obj_clear_flag(lv_obj_get_child(strip,7),LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(lv_obj_get_child(strip,7),LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(lv_obj_get_child(strip,7),lv_color_hex(bambuStatus.external_color2),0);
    lv_obj_set_width(strip, LV_SIZE_CONTENT);
}
static void strip_message(lv_event_t *e){refresh_strip(lv_event_get_target(e));}
static void cycle_strip(lv_event_t *e)
{
    lv_obj_t *strip=lv_event_get_target(e);unsigned current=strip_unit(strip);
    for(unsigned step=1;step<=XTOUCH_MAX_AMS_UNITS;++step){unsigned next=(current+step)%XTOUCH_MAX_AMS_UNITS;
        if(bambuStatus.ams_exist_bits&(1UL<<next)){lv_obj_set_user_data(strip,(void *)(uintptr_t)next);break;}}
    refresh_strip(strip);
}
void ui_ams_strip_bind(lv_obj_t *strip)
{
    /* The label's 19px line box must fit too. A 16px flex line clips the
       centered 16px swatches when an AMS model label becomes visible. */
    lv_obj_set_height(strip, 22);
    lv_obj_set_user_data(strip,NULL);
    lv_obj_t *name=lv_label_create(strip);lv_obj_set_width(name,LV_SIZE_CONTENT);lv_obj_set_flex_grow(name,0);lv_obj_move_to_index(name,0);
    lv_obj_t *separator=lv_label_create(strip);lv_label_set_text(separator,"|");lv_obj_set_width(separator,8);
    lv_obj_t *external=lv_obj_create(strip);lv_obj_remove_style_all(external);lv_obj_set_size(external,16,16);
    lv_obj_set_style_bg_opa(external,LV_OPA_COVER,0);lv_obj_set_style_radius(external,3,0);
    lv_obj_set_style_border_width(external,1,0);lv_obj_set_style_border_color(external,lv_color_hex(0xAAAAAA),0);
    lv_obj_clear_flag(external,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *second=lv_obj_create(strip);lv_obj_remove_style_all(second);lv_obj_set_size(second,16,16);lv_obj_set_style_bg_opa(second,LV_OPA_COVER,0);lv_obj_set_style_radius(second,3,0);lv_obj_set_style_border_width(second,1,0);lv_obj_set_style_border_color(second,lv_color_hex(0xAAAAAA),0);lv_obj_clear_flag(second,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    /* Physical square swatches for every display aspect ratio. */
    const unsigned swatches[]={1,2,3,4,6,7};
    for(unsigned i=0;i<sizeof(swatches)/sizeof(swatches[0]);i++)ui_layout_square(lv_obj_get_child(strip,swatches[i]),16);
    lv_obj_set_style_pad_column(strip,2,0);
    lv_obj_set_flex_align(strip,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    refresh_strip(strip);
    lv_obj_add_event_cb(strip,strip_message,LV_EVENT_MSG_RECEIVED,NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_AMS,strip,NULL);lv_msg_subsribe_obj(XTOUCH_ON_AMS_BITS,strip,NULL);
    lv_obj_add_flag(strip,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(strip,cycle_strip,LV_EVENT_SHORT_CLICKED,NULL);
}

/* Browsing an AMS never changes the printer's active feeder. */
static int selected_unit = -1;
static unsigned selected_printer = 255;
static unsigned panel_base(void)
{
    unsigned printer = xtouch_printer_active();
    if (selected_printer != printer) { selected_unit = -1; selected_printer = printer; }
    if (selected_unit < 0 || !(bambuStatus.ams_exist_bits & (1UL << selected_unit)))
        selected_unit = ui_ams_base() / 4;
    return selected_unit * 4;
}

bool ui_ams_select_unit(unsigned unit)
{
    if (unit >= XTOUCH_MAX_AMS_UNITS || !(bambuStatus.ams_exist_bits & (1UL << unit))) return false;
    panel_base(); selected_unit = unit;
    if (ui_filament_selected() == 254 || ui_filament_selected()/4 != (int)unit) ui_filament_select(-1);
    return true;
}

unsigned ui_ams_selected_unit(void){return panel_base()/4;}
bool ui_ams_drying_available(void){if(ui_filament_selected()>=253)return false;unsigned unit=ui_ams_selected_unit();return (bambuStatus.ams_exist_bits&(1U<<unit))&&(bambuStatus.ams_kind[unit]==3||bambuStatus.ams_kind[unit]==4);}
enum { AMS_HEADER_WIDTH=176, AMS_TAB_MIN_WIDTH=50 };
static int header_width(void){return bambuStatus.external_present?AMS_HEADER_WIDTH:264;}
static unsigned panel_units(unsigned units[XTOUCH_MAX_AMS_UNITS])
{
    unsigned count=0;
    for (unsigned i=0;i<XTOUCH_MAX_AMS_UNITS;++i) if (bambuStatus.ams_exist_bits & (1UL<<i)) units[count++]=i;
    return count;
}
static int header_content_width(unsigned count)
{
    int width=header_width();
    return count>4 && (int)count*AMS_TAB_MIN_WIDTH>width ? (int)count*AMS_TAB_MIN_WIDTH : width;
}
/* A single opening nudge is an animation of scrolling, not a feeder selection.
   LVGL owns its lifetime; deleting the page removes animations for the viewport. */
static void hint_scroll(void *object,int32_t value) {lv_obj_scroll_to_x(object,value,LV_ANIM_OFF);}
static void stop_hint(lv_event_t *e) {lv_anim_del(lv_event_get_user_data(e),hint_scroll);}
static void header_hint(lv_obj_t *viewport,unsigned count)
{
    if(count<=4 || lv_obj_has_flag(viewport,LV_OBJ_FLAG_USER_1))return;
    lv_obj_add_flag(viewport,LV_OBJ_FLAG_USER_1);
    lv_anim_t animation;lv_anim_init(&animation);
    lv_anim_set_var(&animation,viewport);lv_anim_set_exec_cb(&animation,hint_scroll);
    lv_anim_set_values(&animation,0,AMS_TAB_MIN_WIDTH);
    lv_anim_set_delay(&animation,400);lv_anim_set_time(&animation,230);
    lv_anim_set_playback_delay(&animation,80);lv_anim_set_playback_time(&animation,250);
    lv_anim_set_early_apply(&animation,false);lv_anim_set_path_cb(&animation,lv_anim_path_ease_in_out);
    lv_anim_start(&animation);
}
static void header_sync(lv_obj_t *panel)
{
    unsigned units[XTOUCH_MAX_AMS_UNITS], count=panel_units(units);
    lv_obj_t *viewport=lv_obj_get_child(panel,0);
    lv_obj_set_width(viewport,header_width());
    lv_obj_set_width(lv_obj_get_child(viewport,0),header_content_width(count));
    header_hint(viewport,count);
    lv_obj_invalidate(viewport);
}

static void panel_click(lv_event_t *e)
{
    lv_indev_t *input = lv_indev_get_act();
    if (!input) return;
    lv_point_t point; lv_indev_get_point(input, &point);
    lv_obj_t *o = lv_event_get_target(e); lv_area_t area; lv_obj_get_content_coords(o, &area);
    if (point.x < area.x1 || point.x > area.x2 || point.y < area.y1 || point.y > area.y2) return;
    if (point.y <= area.y1+22) {
        if(point.x>=area.x1+176 && bambuStatus.external_present)ui_filament_select(bambuStatus.nozzle_count==2&&bambuStatus.external_multi_protocol&&point.x>=area.x1+220?253:254);
        else return; /* The independent AMS scroll viewport handles this area. */
    } else if (point.y >= area.y1+26 && ui_filament_selected()!=254) {
        int width=xtouch_ams_slot_count(panel_base()/4)==1?lv_area_get_width(&area):(lv_area_get_width(&area)-12)/4;
        int x=point.x-area.x1, index=x/(width+4);
        if (index>3 || x%(width+4)>=width) return;
        if((unsigned)index>=xtouch_ams_slot_count(panel_base()/4))return;
        int tray=panel_base()+index;
        ui_filament_select(ui_filament_selected()==tray ? -1 : tray);
    }
    ui_filament_refresh(lv_obj_get_parent(o));
    lv_obj_invalidate(o);
}

static void details_event(lv_event_t *e){ui_ams_details_show(panel_base()/4);}

static void panel_message(lv_event_t *e) { lv_obj_t *panel=lv_event_get_target(e); header_sync(panel); lv_obj_invalidate(panel); }

static void draw_text(lv_draw_ctx_t *ctx, lv_area_t area, const char *text, uint32_t color)
{
    lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d);
    d.font = LV_FONT_DEFAULT; d.color = lv_color_hex(color); d.align = LV_TEXT_ALIGN_CENTER;
    lv_draw_label(ctx, &d, &area, text, NULL);
}

static void draw_header(lv_event_t *e)
{
    lv_obj_t *content=lv_event_get_target(e); lv_area_t a;lv_obj_get_coords(content,&a);
    lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(e);
    unsigned units[XTOUCH_MAX_AMS_UNITS], count=panel_units(units), base=panel_base();
    int selection=ui_filament_selected();
    if (!count) { draw_text(ctx,a,"AMS --",0xAAAAAA); return; }
    for(unsigned index=0;index<count;++index) {
        unsigned id=units[index];
        lv_area_t tab={a.x1+(int)index*lv_area_get_width(&a)/(int)count,a.y1,
            a.x1+(int)(index+1)*lv_area_get_width(&a)/(int)count-3,a.y1+21};
        bool active=selection<253 && base/4==id;
        lv_draw_rect_dsc_t r;lv_draw_rect_dsc_init(&r);
        r.bg_color=lv_color_hex(active ? 0x777777 : 0x555555);r.bg_opa=LV_OPA_COVER;r.radius=4;
        lv_draw_rect(ctx,&r,&tab);
        char heading[48],unitName[8];xtouch_ams_unit_label(id,unitName,sizeof(unitName));snprintf(heading,sizeof(heading),"%s",unitName);
        if (count==1) {
            size_t n=strlen(heading);
            if (bambuStatus.ams_humidity_valid & (1U<<id))
                n+=snprintf(heading+n,sizeof(heading)-n," %u%%",bambuStatus.ams_humidity_percent[id]);
            else if (bambuStatus.ams_temperature_valid & (1U<<id))
                snprintf(heading+n,sizeof(heading)-n," %.1fC",bambuStatus.ams_temperature_deci[id]/10.0);
        }
        tab.y1+=3;draw_text(ctx,tab,heading,active ? 0xFFFFFF : 0xBBBBBB);
    }
}
static void header_click(lv_event_t *e)
{
    lv_indev_t *input=lv_indev_get_act();if(!input)return;
    lv_point_t point;lv_indev_get_point(input,&point);
    lv_obj_t *content=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(content,&a);
    unsigned units[XTOUCH_MAX_AMS_UNITS],count=panel_units(units);if(!count)return;
    unsigned index=(point.x-a.x1)*count/lv_area_get_width(&a);
    if(index>=count || !ui_ams_select_unit(units[index]))return;
    lv_obj_t *panel=lv_event_get_user_data(e);
    ui_filament_refresh(lv_obj_get_parent(panel));lv_obj_invalidate(panel);
}

/* Draw four fixed cards without eight extra label/container allocations. */
static void draw_panel(lv_event_t *e)
{
    lv_obj_t *o = lv_event_get_target(e); lv_area_t a;
    lv_obj_get_content_coords(o, &a); lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    unsigned base = panel_base();
    int selection = ui_filament_selected();
    /* External remains stationary, outside the horizontal AMS viewport. */
    lv_area_t external={a.x1+176,a.y1,a.x2,a.y1+21};
    lv_draw_rect_dsc_t tab; lv_draw_rect_dsc_init(&tab);
    tab.bg_color=lv_color_hex(selection>=253 ? 0x777777 : 0x555555);
    tab.bg_opa=LV_OPA_COVER; tab.radius=4;
    if(bambuStatus.external_present)lv_draw_rect(ctx,&tab,&external);
    external.y1+=3;
    if(!bambuStatus.external_present) { /* No configured external material. */ }
    else if(bambuStatus.nozzle_count==2&&bambuStatus.external_multi_protocol){
        for(unsigned n=0;n<2;n++){if(!(bambuStatus.external_materials&(1U<<n)))continue;lv_area_t cell=external;cell.x1=a.x1+176+n*44;cell.x2=cell.x1+42;char caption[8];snprintf(caption,sizeof(caption),"EXT%u",n);draw_text(ctx,cell,caption,selection==(n?253:254)?0x39A1FD:0xBBBBBB);}
    }else draw_text(ctx,external,XTOUCH_TEXT_FILAMENT_EXTERNAL,selection>=253?0xFFFFFF:0xBBBBBB);
    if (selection >= 253 || !bambuStatus.ams_exist_bits) {
        draw_text(ctx,(lv_area_t){a.x1,a.y1+31,a.x2,a.y2},
            selection >= 253 ? XTOUCH_TEXT_FILAMENT_EXTERNAL_HELP : XTOUCH_TEXT_FILAMENT_PICK,0xCCCCCC);
        return;
    }
    unsigned slots=xtouch_ams_slot_count(base/4);
    int width = slots==1?lv_area_get_width(&a):(lv_area_get_width(&a) - 12) / 4;
    for (unsigned i = 0; i < xtouch_ams_slot_count(base/4); ++i) {
        unsigned slot = base + i;
        lv_area_t card = {a.x1 + (int)i * (width + 4), a.y1 + 26,
                         a.x1 + (int)i * (width + 4) + width - 1, a.y2};
        lv_draw_rect_dsc_t r; lv_draw_rect_dsc_init(&r);
        r.bg_color = lv_color_hex(0x555555); r.bg_opa = LV_OPA_COVER;
        r.radius = 6; r.border_width = selection == (int)slot ? 3 : 1;
        r.border_color = lv_color_hex(selection == (int)slot ? 0x39A1FD : slot_border(slot));
        lv_draw_rect(ctx, &r, &card);
        // A real color swatch remains legible even when the filament is dark.
        lv_area_t swatch = {card.x1 + 8, card.y1 + 5, card.x2 - 8, card.y1 + 13};
        r.bg_color = lv_color_hex(ui_ams_slot_color(slot)); r.border_width = 1;
        r.border_color = lv_color_hex(slot_border(slot)); r.radius = 2;
        lv_draw_rect(ctx, &r, &swatch);
        char text[80], remaining[24] = "";
        if (bambuStatus.ams_remain_valid & (1ULL << slot))
            snprintf(remaining,sizeof(remaining)," #%06x %u%%#", (unsigned)ui_ams_remaining_color(bambuStatus.ams_remain_percent[slot]), bambuStatus.ams_remain_percent[slot]);
        char source[20];
        if(base/4==XTOUCH_MIXED_AMS_UNIT)snprintf(source,sizeof(source),"L%u",i+1);
        else if(base/4<4)snprintf(source,sizeof(source),"%c%u",'A'+base/4,i+1);
        else snprintf(source,sizeof(source),"HT-%u",base/4-3);
        snprintf(text,sizeof(text),"#91C7E5 %s#%s\n%s",source,remaining,bambuStatus.ams&&bambuStatus.ams_tray_material[slot][0]?bambuStatus.ams_tray_material[slot]:"--");
        lv_area_t caption = {card.x1 + 3, card.y1 + 17, card.x2 - 3, card.y2 - 3};
        lv_draw_label_dsc_t caption_style; lv_draw_label_dsc_init(&caption_style);
        caption_style.font = LV_FONT_DEFAULT;
        caption_style.color = lv_color_white();
        caption_style.align = LV_TEXT_ALIGN_CENTER;
        caption_style.flag = LV_TEXT_FLAG_RECOLOR;
        lv_draw_label(ctx, &caption_style, &caption, text, NULL);
    }
}

static void position(lv_obj_t *o, int x, int y, int w, int h)
{
    // Generated flex widgets may have been measured during textarea creation.
    o->w_layout = 0; o->h_layout = 0;
    lv_obj_set_flex_grow(o, 0); lv_obj_set_align(o, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(o, x, y); lv_obj_set_size(o, w, h);
}

void ui_ams_filament_layout(lv_obj_t *component)
{
    lv_obj_set_layout(component, 0);
    lv_obj_t *nozzle = ui_comp_get_child(component, 1);
    lv_obj_t *filament = ui_comp_get_child(component, 6);
    position(nozzle, 0, 88, 126, 144);
    position(filament, 130, 88, 134, 144);
    lv_obj_set_style_pad_row(nozzle, 4, 0);
    lv_obj_set_style_pad_row(filament, 4, 0);
    const unsigned controls[] = {2, 3, 5, 7, 8, 9};
    for (unsigned i = 0; i < 6; ++i) {
        lv_obj_t *control = ui_comp_get_child(component, controls[i]);
        control->h_layout = 0;
        lv_obj_set_flex_grow(control, 0);
        lv_obj_set_height(control, 45);
        lv_obj_set_style_pad_top(control, controls[i] == 7 || controls[i] == 9 ? 16 : 8, 0);
        lv_obj_set_style_pad_bottom(control, 0, 0);
    }
    /* Center icon and temperature as one compact horizontal group. */
    lv_obj_t *group = ui_comp_get_child(component, 3);
    lv_obj_t *temperature = ui_comp_get_child(component, 4);
    lv_label_set_text(group, "");
    lv_obj_set_style_pad_all(group, 0, 0);
    lv_obj_set_style_pad_top(temperature, 0, 0);
    lv_obj_set_flex_align(group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(group, 4, 0);
    lv_obj_t *icon = lv_label_create(group);
    lv_label_set_text(icon, "p");
    lv_obj_set_style_text_font(icon, &ui_font_xlcd, 0);
    lv_obj_move_to_index(icon, 0);
    // Align visible glyph ink, not unequal font line boxes.
    lv_font_glyph_dsc_t a, b;
    if (lv_font_get_glyph_dsc(&ui_font_xlcd, &a, 'p', 0) &&
        lv_font_get_glyph_dsc(XTOUCH_FONT_NUMERIC, &b, '0', 0)) {
        int icon_center = ui_font_xlcd.line_height - 2*ui_font_xlcd.base_line - 2*a.ofs_y - a.box_h;
        int text_center = (XTOUCH_FONT_NUMERIC)->line_height - 2*(XTOUCH_FONT_NUMERIC)->base_line - 2*b.ofs_y - b.box_h;
        lv_obj_set_style_translate_y(temperature, (icon_center-text_center)/2, 0);
    }

    lv_obj_t *panel = lv_obj_create(component);
    lv_obj_remove_style_all(panel); position(panel, 0, 0, 264, 80);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *viewport=lv_obj_create(panel);
    lv_obj_remove_style_all(viewport);position(viewport,0,0,AMS_HEADER_WIDTH,24);
    lv_obj_set_scroll_dir(viewport,LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(viewport,LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_height(viewport,2,LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(viewport,lv_color_hex(0xAAAAAA),LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(viewport,LV_OPA_50,LV_PART_SCROLLBAR);
    lv_obj_clear_flag(viewport,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_t *content=lv_obj_create(viewport);
    lv_obj_remove_style_all(content);position(content,0,0,AMS_HEADER_WIDTH,22);
    lv_obj_clear_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_add_flag(content,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(content,stop_hint,LV_EVENT_PRESSED,viewport);
    lv_obj_add_event_cb(content,draw_header,LV_EVENT_DRAW_MAIN_END,NULL);
    lv_obj_add_event_cb(content,header_click,LV_EVENT_SHORT_CLICKED,panel);
    lv_obj_add_event_cb(content,details_event,LV_EVENT_LONG_PRESSED,NULL);
    header_sync(panel);
    lv_obj_add_event_cb(panel, panel_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(panel, draw_panel, LV_EVENT_DRAW_MAIN_END, NULL);
    lv_obj_add_event_cb(panel, panel_message, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_AMS, panel, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_AMS_BITS, panel, NULL);
}
