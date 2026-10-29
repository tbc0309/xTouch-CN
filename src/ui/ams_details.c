#include "ams_details.h"
#include "ams_display.h"
#include "detail_page.h"
#include "filament_actions.h"
#include "i18n/language.h"
#include "numeric_editor.h"
#include "message_style.h"
#include "xtouch/ams_units.h"
#include <stdio.h>

static lv_obj_t *dialog, *title, *readings, *temperature, *duration, *start, *stop;
static unsigned unit, printer, draftTemp = 55, draftHours = 4, draftUnit = 255, draftOwner = 255;
static struct ui_dry_request pending;
bool ui_ams_dry_allowed(const struct ui_dry_request *r) {
    if (xtouch_command_pending() || !r || r->printer != xtouch_printer_active() ||
        !xtouch_printer_online() || xtouch_printer_switch_pending() || bambuStatus.non_fdm_mode ||
        r->unit >= XTOUCH_MAX_AMS_UNITS)
        return false;
    if (!(bambuStatus.ams_exist_bits & (1U << r->unit)) ||
        !(bambuStatus.ams_dry_valid & (1U << r->unit)))
        return false;
    unsigned kind = bambuStatus.ams_kind[r->unit];
    if (kind != 3 && kind != 4)
        return false;
    if (r->stop)
        return bambuStatus.ams_dry_status[r->unit] >= 1 && bambuStatus.ams_dry_status[r->unit] <= 3;
    return r->temp >= 45 && r->temp <= (kind == 4 ? 85 : 65) && r->hours >= 1 && r->hours <= 24 &&
           bambuStatus.ams_dry_status[r->unit] == 0 &&
           (bambuStatus.print_status == XTOUCH_PRINT_STATUS_IDLE ||
            bambuStatus.print_status == XTOUCH_PRINT_STATUS_FINISHED ||
            bambuStatus.print_status == XTOUCH_PRINT_STATUS_FAILED);
}
static void confirm_dry(void) {
    if (ui_ams_dry_allowed(&pending))
        lv_msg_send(XTOUCH_COMMAND_AMS_DRY, &pending);
}
static void action(lv_event_t *e) {
    pending = (struct ui_dry_request){printer, unit, draftTemp, draftHours,
                                      lv_event_get_target_obj(e) == stop};
    if (!ui_ams_dry_allowed(&pending))
        return;
    char text[100];
    snprintf(text, sizeof(text), pending.stop ? "%s %s?" : "%s %s %u℃ / %uh?",
             xtouch_ams_kind_name(unit),
             pending.stop ? (xtouch_local("停止烘干", "Stop drying"))
                          : (xtouch_local("开始烘干", "Start drying")),
             pending.temp, pending.hours);
    ui_confirmPanel_show(text, confirm_dry);
}
static void close_dialog(lv_event_t *e) { ui_detail_page_end(false); }
static void deleted(lv_event_t *e) { dialog = NULL; }
static void refresh(lv_event_t *e) {
    if (printer != xtouch_printer_active()) {
        close_dialog(e);
        return;
    }
    char name[8], text[190];
    xtouch_ams_unit_label(unit, name, sizeof(name));
    snprintf(text, sizeof(text), "%s · %s", xtouch_ams_kind_name(unit), name);
    lv_label_set_text(title, text);
    ui_detail_page_title(dialog,title);
    const char *states[] = {"Off",      "Checking", "Drying", "Cooling",
                            "Stopping", "Error",    "Error",  "Test"};
    const char *zh[] = {"已停止", "检查中", "烘干中", "冷却中", "停止中", "故障", "故障", "测试"};
    unsigned status = bambuStatus.ams_dry_status[unit];
    char temp[20] = "--", humidity[20] = "--", time[32] = "--";
    if (bambuStatus.ams_temperature_valid & (1U << unit))
        snprintf(temp, sizeof(temp), "%.1f℃", bambuStatus.ams_temperature_deci[unit] / 10.0);
    if (bambuStatus.ams_humidity_valid & (1U << unit))
        snprintf(humidity, sizeof(humidity), "%u%%", bambuStatus.ams_humidity_percent[unit]);
    if (bambuStatus.ams_dry_valid & (1U << unit))
        snprintf(time, sizeof(time), "%uh %umin", bambuStatus.ams_dry_minutes[unit] / 60,
                 bambuStatus.ams_dry_minutes[unit] % 60);
    snprintf(text, sizeof(text), "%s    %s\n%s  ·  %s\n%s %u℃", temp, humidity,
             (bambuStatus.ams_dry_valid & (1U << unit))
                 ? (status < 8 ? (xtouch_local(zh[status], states[status])) : "--")
                 : "--",
             time, xtouch_local("目标", "Target"), bambuStatus.ams_dry_target[unit]);
    if (bambuStatus.ams_kind[unit] != 3 && bambuStatus.ams_kind[unit] != 4)
        snprintf(text, sizeof(text), "%s    %s", temp, humidity);
    lv_label_set_text(readings, text);
    struct ui_dry_request request = {printer, unit, draftTemp, draftHours, false};
    if (ui_ams_dry_allowed(&request))
        lv_obj_clear_state(start, LV_STATE_DISABLED);
    else
        lv_obj_add_state(start, LV_STATE_DISABLED);
    request.stop = true;
    if (ui_ams_dry_allowed(&request))
        lv_obj_clear_state(stop, LV_STATE_DISABLED);
    else
        lv_obj_add_state(stop, LV_STATE_DISABLED);
}
static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y) {
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, text);
    lv_obj_set_pos(o, x, y);
    return o;
}
static void return_details(void) {
    unsigned selected = unit;
    loadScreen(3);
    ui_ams_details_show(selected);
}
static void numeric_apply(unsigned index, unsigned value) {
    if (printer != xtouch_printer_active())
        return;
    if (index)
        draftHours = value;
    else
        draftTemp = value;
    return_details();
}
static void edit_parameter(lv_event_t *e) {
    struct ui_numeric_field fields[] = {
        {xtouch_local("温度", "Temperature"), draftTemp, 45,
         bambuStatus.ams_kind[unit] == 4 ? 85 : 65},
        {xtouch_local("小时", "Hours"), draftHours, 1, 24}};
    ui_numeric_show(fields, 2, (uintptr_t)lv_event_get_user_data(e), numeric_apply, return_details,
                    NULL);
}
void ui_ams_details_show(unsigned selected) {
    if (selected >= XTOUCH_MAX_AMS_UNITS || !(bambuStatus.ams_exist_bits & (1U << selected)))
        return;
    ui_detail_page_end(false);
    unit = selected;
    printer = xtouch_printer_active();
    if (draftUnit != unit || draftOwner != printer) {
        draftUnit = unit;
        draftOwner = printer;
        unsigned value = bambuStatus.ams_dry_target[unit];
        draftTemp =
            value >= 45 && value <= (bambuStatus.ams_kind[unit] == 4 ? 85 : 65) ? value : 55;
        draftHours = 4;
    }
    dialog = ui_detail_page_begin();
    if (!dialog)
        return;
    lv_obj_add_event_cb(dialog, deleted, LV_EVENT_DELETE, NULL);
    title = label(dialog, "", 0, 0);
    ui_detail_page_title(dialog,title);
    readings = label(dialog, "", 4, 38);
    lv_obj_set_style_text_line_space(readings, 8, 0);
    lv_obj_t *parameterHeading =
        label(dialog, xtouch_local("温度 / 小时", "Temperature / hours"), 4, 119);
    temperature = lv_btn_create(dialog);
    duration = lv_btn_create(dialog);
    ui_action_style(temperature,UI_ACTION_SECONDARY);
    ui_action_style(duration,UI_ACTION_SECONDARY);
    char value[24];
    snprintf(value, sizeof(value), "%u℃", draftTemp);
    label(temperature, value, 0, 0);
    snprintf(value, sizeof(value), "%uh", draftHours);
    label(duration, value, 0, 0);
    lv_obj_add_event_cb(temperature, edit_parameter, LV_EVENT_CLICKED, (void *)0);
    lv_obj_add_event_cb(duration, edit_parameter, LV_EVENT_CLICKED, (void *)1);
    start = lv_btn_create(dialog);
    label(start, xtouch_local("开始烘干", "Start drying"), 0, 0);
    lv_obj_add_event_cb(start, action, LV_EVENT_CLICKED, NULL);
    stop = lv_btn_create(dialog);
    label(stop, xtouch_local("停止烘干", "Stop drying"), 0, 0);
    lv_obj_add_event_cb(stop, action, LV_EVENT_CLICKED, NULL);
    ui_action_style(start,UI_ACTION_COMPLETE);
    ui_action_style(stop,UI_ACTION_DESTRUCTIVE);
    for(unsigned i=0;i<2;++i){lv_obj_t *button=i?stop:start;lv_obj_set_style_bg_color(button,lv_color_hex(0x555555),LV_STATE_DISABLED);lv_obj_set_style_text_color(button,lv_color_hex(0x888888),LV_STATE_DISABLED);}
    ui_detail_page_block(dialog,readings,38,0);
    ui_detail_page_block(dialog,parameterHeading,119,0);
    ui_detail_page_split(dialog,temperature,duration,142,32,1,1);
    ui_detail_page_split(dialog,start,stop,184,32,1,1);
    if (bambuStatus.ams_kind[unit] != 3 && bambuStatus.ams_kind[unit] != 4) {
        lv_obj_t *controls[] = {parameterHeading, temperature, duration, start, stop};
        for (unsigned i = 0; i < 5; i++)
            lv_obj_add_flag(controls[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_msg_subsribe_obj(XTOUCH_ON_AMS, dialog, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_MQTT, dialog, NULL);
    lv_obj_add_event_cb(dialog, refresh, LV_EVENT_MSG_RECEIVED, NULL);
    refresh(NULL);
    ui_layout_captions_tree(dialog);
}
