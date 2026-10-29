#include "filament_actions.h"
#include "ams_details.h"
#include "ams_display.h"
#include "i18n/language.h"
#include "printer_models.h"
#include "xtouch/ams_units.h"
#include <stdio.h>
#include <string.h>

/* Selection is a UI target, never a change to the printer's loaded feeder. */
static int selected = -1;
static unsigned selected_printer = 255;
static struct ui_filament_request pending;

static bool slot_exists(int tray) {
    return (tray == 254 && bambuStatus.external_present &&
            (bambuStatus.nozzle_count < 2 || ((bambuStatus.external_routes & bambuStatus.external_materials) & 1))) ||
           (tray == 253 && bambuStatus.nozzle_count == 2 && bambuStatus.external_multi_protocol &&
            (bambuStatus.external_routes & bambuStatus.external_materials & 2)) ||
           (tray >= 0 && tray < XTOUCH_MAX_AMS_SLOTS &&
            (bambuStatus.ams_exist_bits & (1UL << (tray / 4))) &&
            (bambuStatus.tray_exist_bits & (1ULL << tray)));
}
int ui_filament_selected(void) {
    unsigned printer = xtouch_printer_active();
    if (printer != selected_printer) {
        selected = -1;
        selected_printer = printer;
    }
    if (selected >= 0 && !slot_exists(selected))
        selected = -1;
    return selected;
}
bool ui_filament_select(int tray) {
    ui_filament_selected();
    if (tray != -1 && !slot_exists(tray))
        return false;
    selected = tray;
    return true;
}
int ui_filament_extruder(int tray) {
    if (bambuStatus.nozzle_count < 2)
        return 0;
    if (tray == 254 || tray == 253) {
        int id = tray == 254 ? 0 : 1;
        return bambuStatus.external_multi_protocol && (bambuStatus.external_routes & (1U << id)) &&
                       (bambuStatus.nozzle_valid & (1U << id)) &&
                       (bambuStatus.nozzle_loaded_valid & (1U << id))
                   ? id
                   : -1;
    }
    if (tray < 0 || tray >= XTOUCH_MAX_AMS_SLOTS)
        return -1;
    unsigned id = bambuStatus.ams_bound_extruder[tray / 4];
    return id < 2 && (bambuStatus.nozzle_valid & (1U << id)) &&
                   (bambuStatus.nozzle_loaded_valid & (1U << id))
               ? (int)id
               : -1;
}
int ui_filament_current(int tray) {
    int id = ui_filament_extruder(tray);
    return id < 0 ? -1
           : bambuStatus.nozzle_count && (bambuStatus.nozzle_loaded_valid & (1U << id))
               ? bambuStatus.nozzle_loaded[id]
               : bambuStatus.m_tray_now;
}
bool ui_filament_is_loaded(int tray) {
    if (bambuStatus.nozzle_count && bambuStatus.nozzle_loaded_valid) {
        for (unsigned id = 0; id < bambuStatus.nozzle_count && id < 2; ++id)
            if ((bambuStatus.nozzle_loaded_valid & (1U << id)) &&
                bambuStatus.nozzle_loaded[id] == tray)
                return true;
        return false;
    }
    return bambuStatus.m_tray_now == tray;
}
static bool supported_route(int tray) {
    if (tray >= 0 && tray < XTOUCH_MAX_AMS_SLOTS && bambuStatus.ams_kind[tray / 4] == 255)
        return false;
    const char *m = ui_printer_model_code(bambuStatus.printer_type);
    if (!m || !*m)
        return false;
    if (bambuStatus.nozzle_count) {
        int id = ui_filament_extruder(tray);
        return id >= 0 && (bambuStatus.nozzle_valid & (1U << id));
    }
    return !strcmp(m, "C11") || !strcmp(m, "C12") || !strcmp(m, "C13") || !strcmp(m, "BL-P001") ||
           !strcmp(m, "BL-P002") || !strcmp(m, "N1") || !strcmp(m, "N2S");
}
int ui_filament_temperature(int tray) {
    unsigned low = 0, high = 0;
    if (tray == 254) {
        low = bambuStatus.external_nozzle_min;
        high = bambuStatus.external_nozzle_max;
    } else if (tray == 253) {
        low = bambuStatus.external_nozzle_min2;
        high = bambuStatus.external_nozzle_max2;
    } else if (tray >= 0 && tray < XTOUCH_MAX_AMS_SLOTS) {
        low = bambuStatus.ams_nozzle_min[tray];
        high = bambuStatus.ams_nozzle_max[tray];
    }
    if (low >= 150 && high >= low && high <= 350 && (low + high) / 2 <= 300)
        return (low + high) / 2;
    int id = ui_filament_extruder(tray);
    int target = id >= 0 && bambuStatus.nozzle_count ? bambuStatus.nozzle_target[id]
                                                     : bambuStatus.nozzle_target_temper;
    return target >= 150 && target <= 300 ? target : 0;
}
bool ui_filament_allowed(unsigned action, int tray) {
    if (xtouch_command_pending() || bambuStatus.non_fdm_mode || !xtouch_printer_online() ||
        xtouch_printer_switch_pending() || !supported_route(tray) || !slot_exists(tray))
        return false;
    /* Convert only known physical unit/slot mappings before issuing motion. */
    if (tray != 254 && tray != 253 && xtouch_ams_protocol_tray(tray) < 0)
        return false;
    if (bambuStatus.print_status != XTOUCH_PRINT_STATUS_IDLE &&
        bambuStatus.print_status != XTOUCH_PRINT_STATUS_FINISHED &&
        bambuStatus.print_status != XTOUCH_PRINT_STATUS_FAILED)
        return false;
    bool assist_ready = bambuStatus.ams_status_main == AMS_STATUS_MAIN_ASSIST &&
                        bambuStatus.ams_status_sub == 0 && bambuStatus.hw_switch_state == 1 &&
                        bambuStatus.m_tray_now >= 0 &&
                        bambuStatus.m_tray_now < XTOUCH_MAX_AMS_SLOTS &&
                        xtouch_ams_protocol_tray(bambuStatus.m_tray_now) >= 0 &&
                        bambuStatus.m_tray_tar == bambuStatus.m_tray_now;
    /* Loaded AMS normally remains in assist (0x0300), not idle. */
    if (bambuStatus.ams_exist_bits && bambuStatus.ams_status_main != AMS_STATUS_MAIN_IDLE &&
        !assist_ready)
        return false;
    if (bambuStatus.tray_reading_bits || bambuStatus.extruder_busy)
        return false;
    int current = ui_filament_current(tray);
    if (action == UI_FILAMENT_REREAD)
        return tray < XTOUCH_MAX_AMS_SLOTS && current == 255 &&
               bambuStatus.ams_kind[tray / 4] != 2 && bambuStatus.ams_kind[tray / 4] != 5;
    if (action == UI_FILAMENT_UNLOAD)
        return current == tray ||
               (tray == 254 && current == 255 && bambuStatus.hw_switch_state == 1);
    if (action == UI_FILAMENT_LOAD)
        return ui_filament_temperature(tray) > 0 && current != tray &&
               (current == 255 ||
                (current >= 0 && xtouch_ams_protocol_tray(current) >= 0 && current < 254) ||
                current == 254 || current == 253);
    return false;
}
static void target_name(int tray, char *text, size_t capacity) {
    if (tray == 253 || tray == 254)
        snprintf(text, capacity, bambuStatus.nozzle_count == 2 ? "%s T%u" : "%s",
                 XTOUCH_TEXT_FILAMENT_EXTERNAL, tray == 253 ? 1 : 0);
    else if (tray >= XTOUCH_MIXED_AMS_UNIT * 4)
        snprintf(text, capacity, "L-%u-%u",
                 bambuStatus.mixed_ams_protocol_id >= 16 ? bambuStatus.mixed_ams_protocol_id - 15
                                                         : 1,
                 tray % 4 + 1);
    else if (tray < 16)
        snprintf(text, capacity, "%c%d", 'A' + tray / 4, tray % 4 + 1);
    else
        snprintf(text, capacity, "HT%u", tray / 4 - 3);
}
static void confirm_action(void) {
    /* Never redirect an open confirmation after device/selection changes. */
    if (pending.printer != xtouch_printer_active() || pending.tray != ui_filament_selected() ||
        !ui_filament_allowed(pending.action, pending.tray))
        return;
    lv_msg_send(XTOUCH_COMMAND_FILAMENT_ACTION, &pending);
}
void ui_filament_report_status(unsigned status) {
    // Sending/accepting is not completion. Let the live device state speak.
    if (status == 2)
        ui_notice_show(xtouch_local("等待超时，请检查设备状态","Timed out. Check the device state."),0);
}
void ui_filament_report_rejection(uint32_t error, bool authorization) {
    char prompt[240];
    const char *advice=XTOUCH_TEXT_CONTROL_AUTH_ERROR;
    const char *hint=strchr(advice,'\n');
    if(error==84033543UL){
        authorization=true;
        snprintf(prompt,sizeof(prompt),"%.*s\n#%06X %s#",hint?(int)(hint-advice):(int)strlen(advice),advice,
                 (unsigned)UI_COLOR_WARNING,hint?hint+1:advice);
    }
    else {
        if(error)snprintf(prompt,sizeof(prompt),XTOUCH_TEXT_CONTROL_ERROR,(unsigned)error);
        else snprintf(prompt,sizeof(prompt),"%s",xtouch_local("控制请求被拒绝","Control request rejected."));
        if(authorization){size_t n=strlen(prompt);snprintf(prompt+n,sizeof(prompt)-n,"\n#%06X %s#",(unsigned)UI_COLOR_WARNING,hint?hint+1:advice);}
    }
    ui_notice_show_kind(prompt,authorization?4000:0,authorization?UI_MESSAGE_WARNING:UI_MESSAGE_ERROR);
}
void ui_filament_report_send_failed(void){ui_notice_show(xtouch_local("控制请求发送失败，请检查连接","Request could not be sent. Check the connection."),0);}
void ui_filament_report_error(uint32_t error) {
    if(error)ui_filament_report_rejection(error,error==84033543UL);
}

void ui_filament_request_action(unsigned action) {
    int tray = ui_filament_selected();
    if (!ui_filament_allowed(action, tray))
        return;
    pending = (struct ui_filament_request){xtouch_printer_active(), tray, action};
    char name[24], prompt[180];
    target_name(tray, name, sizeof(name));
    const char *format = action == UI_FILAMENT_LOAD     ? XTOUCH_TEXT_FILAMENT_LOAD_CONFIRM
                         : action == UI_FILAMENT_UNLOAD ? XTOUCH_TEXT_FILAMENT_UNLOAD_CONFIRM
                                                        : XTOUCH_TEXT_FILAMENT_REREAD_CONFIRM;
    snprintf(prompt, sizeof(prompt), format, name);
    ui_confirmPanel_show(prompt, confirm_action);
}
void ui_filament_refresh(lv_obj_t *component) {
    int tray = ui_filament_selected();
    char name[24], caption[48];
    if (tray >= 0)
        target_name(tray, name, sizeof(name));
    else
        name[0] = 0;
    const unsigned ids[] = {
        UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENLOAD,
        UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENUNLOAD,
        UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENFILAMENTICON};
    const char *labels[] = {XTOUCH_TEXT_LOAD, XTOUCH_TEXT_UN_NLOAD, XTOUCH_TEXT_FILAMENT_REREAD};
    for (unsigned action = 0; action < 3; ++action) {
        lv_obj_t *button = ui_comp_get_child(component, ids[action]);
        bool split = action == 2 && ui_ams_drying_available();
        bool allowed = ui_filament_allowed(action, tray) || split;
        if (allowed)
            lv_obj_clear_state(button, LV_STATE_DISABLED);
        else
            lv_obj_add_state(button, LV_STATE_DISABLED);
        if (tray < 0)
            snprintf(caption, sizeof(caption), "%s",
                     action == 2 ? labels[action] : XTOUCH_TEXT_FILAMENT_PICK);
        else
            snprintf(caption, sizeof(caption), "%s %s", name,
                     action == UI_FILAMENT_LOAD && !ui_filament_temperature(tray)
                         ? XTOUCH_TEXT_FILAMENT_SET_TEMP
                         : labels[action]);
        lv_obj_t *label = action == 2 ? lv_obj_get_child(button, 0) : button;
        if (action == 2) {
            lv_obj_set_style_bg_opa(button, split ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
            lv_obj_set_style_bg_opa(button, split ? LV_OPA_TRANSP : LV_OPA_COVER, LV_STATE_PRESSED);
            lv_label_set_text(button, "");
            lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(button);
        }
        if (strcmp(lv_label_get_text(label), caption))
            lv_label_set_text(label, caption);
        if(action!=2)ui_layout_button_text(button);
    }
}
static void refresh_event(lv_event_t *e) { ui_filament_refresh(lv_event_get_target_obj(e)); }
static void reread_click(lv_event_t *e) {
    if (ui_ams_drying_available()) {
        lv_indev_t *input = lv_indev_get_act();
        if (!input)
            return;
        lv_point_t point;
        lv_indev_get_point(input, &point);
        lv_area_t area;
        lv_obj_get_coords(lv_event_get_target_obj(e), &area);
        if (point.x >= area.x1 + lv_area_get_width(&area) / 2) {
            ui_ams_details_show(ui_ams_selected_unit());
            return;
        }
    }
    ui_filament_request_action(UI_FILAMENT_REREAD);
}
static void draw_reread(lv_event_t *e) {
    lv_obj_t *button = lv_event_get_target_obj(e);
    lv_area_t a;
    ui_layout_native_bounds(button, &a);
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    unsigned width = lv_area_get_width(&a);
    bool ready = ui_filament_allowed(UI_FILAMENT_REREAD, ui_filament_selected());
    if(!ui_ams_drying_available()){
        const char *text=lv_label_get_text(lv_obj_get_child(button,0));
        lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);
        d.color=lv_obj_get_style_text_color(button,LV_PART_MAIN);
        d.font=ui_layout_canvas_font(&ui_font_xlcdmin,"n");
        lv_area_t icon=ui_layout_ink(d.font,"n");int ih=lv_area_get_height(&icon);
        const lv_font_t *font=ui_layout_canvas_font(LV_FONT_DEFAULT,text);
        lv_area_t ink=ui_layout_ink(font,text);int th=lv_area_get_height(&ink),gap=LV_MAX(2,font->line_height/5);
        int top=a.y1+(lv_area_get_height(&a)-ih-gap-th)/2;
        ui_layout_draw_native_text(ctx,&d,(lv_area_t){a.x1,top,a.x2,top+ih-1},"n",true);
        d.font=font;ui_layout_draw_native_text(ctx,&d,(lv_area_t){a.x1,top+ih+gap,a.x2,top+ih+gap+th-1},text,true);
        return;
    }
    int gap=LV_MAX(2,lv_disp_get_hor_res(NULL)*4/320);
    for (unsigned side = 0; side < 2; side++) {
        lv_area_t cell = a;
        cell.x1 = side ? a.x1+(width+gap)/2 : a.x1;
        cell.x2 = side ? a.x2 : a.x1+(width-gap)/2-1;
        lv_draw_rect_dsc_t r;
        lv_draw_rect_dsc_init(&r);
        r.bg_opa = LV_OPA_COVER;
        r.bg_color = lv_color_hex(0x555555);
        r.radius = lv_disp_get_ver_res(NULL)*6/240;
        ui_layout_native_rect(ctx, &r, &cell);
        lv_draw_label_dsc_t label;
        lv_draw_label_dsc_init(&label);
        const char *text=side ? xtouch_local("烘干", "Dry") : xtouch_local("重读", "RFID");
        label.font = ui_layout_canvas_font(LV_FONT_DEFAULT,text);
        label.align = LV_TEXT_ALIGN_CENTER;
        label.color = lv_color_hex(side ? UI_COLOR_BLUE : ready ? 0xFFFFFF : 0x777777);
        ui_layout_draw_native_text(ctx,&label,cell,text,true);
    }
}
void ui_filament_bind(lv_obj_t *component) {
    lv_obj_t *reread = ui_comp_get_child(
        component, UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENFILAMENTICON);
    lv_obj_add_flag(reread, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(reread, &ui_font_xlcdmin, 0);
    lv_obj_set_style_pad_all(reread, 0, 0);
    lv_obj_set_style_pad_top(reread, 1, 0);
    lv_obj_set_style_text_color(reread, lv_color_hex(0x777777), LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(reread, lv_color_hex(0x444444), LV_STATE_DISABLED);
    lv_obj_t *caption = lv_label_create(reread);
    lv_obj_set_size(caption, lv_pct(100), 18);
    lv_obj_set_pos(caption, 0, 24);
    lv_obj_set_style_text_font(caption, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(caption, LV_LABEL_LONG_CLIP);
    lv_obj_add_event_cb(reread, reread_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(reread, draw_reread, LV_EVENT_DRAW_MAIN_END, NULL);
    const unsigned messages[] = {XTOUCH_ON_AMS, XTOUCH_ON_AMS_BITS, XTOUCH_ON_PRINT_STATUS,
                                 XTOUCH_ON_MQTT};
    for (unsigned i = 0; i < 4; ++i)
        lv_msg_subsribe_obj(messages[i], component, NULL);
    lv_obj_add_event_cb(component, refresh_event, LV_EVENT_MSG_RECEIVED, NULL);
    ui_filament_refresh(component);
}
