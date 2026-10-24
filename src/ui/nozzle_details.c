#include "nozzle_details.h"
#include "detail_page.h"
#include "filament_actions.h"
#include "i18n/language.h"
#include "numeric_editor.h"
#include "printer_models.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static lv_obj_t *dialog, *readings, *indexChoice, *actionChoice, *applyButton;
static unsigned printer;
static struct ui_nozzle_request pending;
static unsigned actionMap[6], actionCount, menuFlags = 255;
static unsigned selected_action(void) {
    unsigned n = lv_dropdown_get_selected(actionChoice);
    return n < actionCount ? actionMap[n] : 99;
}
static bool idle(void) {
    return (bambuStatus.print_status == XTOUCH_PRINT_STATUS_IDLE ||
            bambuStatus.print_status == XTOUCH_PRINT_STATUS_FINISHED ||
            bambuStatus.print_status == XTOUCH_PRINT_STATUS_FAILED) &&
           !bambuStatus.extruder_busy && !bambuStatus.tray_reading_bits &&
           bambuStatus.ams_status_main == AMS_STATUS_MAIN_IDLE;
}
bool ui_nozzle_request_allowed(const struct ui_nozzle_request *r) {
    if (xtouch_command_pending() || !r || r->printer != xtouch_printer_active() ||
        !xtouch_printer_online() || xtouch_printer_switch_pending() || bambuStatus.non_fdm_mode)
        return false;
    if (r->action == 9)
        return xtouch_bblp_chamber_heater() && bambuStatus.chamber_target_valid &&
               r->target <= xtouch_bblp_chamber_max() &&
               (!r->target || r->target >= bambuStatus.chamber_edit_min);
    if (r->action >= 4) {
        bool model =
            !strcmp(bambuStatus.printer_type, "O1C") || !strcmp(bambuStatus.printer_type, "O1C2");
        if (!model || !idle() || !bambuStatus.rack_present || bambuStatus.rack_status ||
            bambuStatus.rack_reading)
            return false;
        if (r->action == 4 || r->action == 5)
            return r->target == 255 ? bambuStatus.rack_mask != 0
                                    : r->target < 16 && (bambuStatus.rack_mask & (1U << r->target));
        return r->action >= 6 && r->action <= 8;
    }
    if (r->index >= bambuStatus.nozzle_count || r->index >= 2 ||
        !(bambuStatus.nozzle_valid & (1U << r->index)))
        return false;
    if (r->action == 0)
        return r->target <= 300;
    if (!idle())
        return false;
    if (r->action == 1)
        return bambuStatus.nozzle_count == 2 && bambuStatus.active_nozzle < 2 &&
               r->index != bambuStatus.active_nozzle;
    return (r->action == 2 || r->action == 3) && bambuStatus.nozzle_actual[r->index] >= 170;
}
static lv_obj_t *label(lv_obj_t *p, const char *text, int x, int y) {
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, text);
    lv_obj_set_pos(l, x, y);
    return l;
}
static void deleted(lv_event_t *e) { dialog = NULL; }
static void confirm(void) {
    if (ui_nozzle_request_allowed(&pending))
        lv_msg_send(XTOUCH_COMMAND_NOZZLE_INDEXED, &pending);
}
static unsigned numericIndices[3], numericOwner, requestedNumeric = 255;
static void numeric_done(unsigned index, unsigned value) {
    pending = (struct ui_nozzle_request){numericOwner, numericIndices[index], value,
                                         numericIndices[index] == 9 ? 9 : 0};
    if (!ui_nozzle_request_allowed(&pending)) {
        ui_notice_unavailable();
        return;
    }
    char text[48];
    if (pending.action == 9)
        snprintf(text, sizeof(text), "%s %u℃?", xtouch_local("仓温", "Chamber"), value);
    else
        snprintf(text, sizeof(text), "T%u %u℃?", pending.index, value);
    ui_confirmPanel_show(text, confirm);
}
static void edit_event(lv_event_t *e) {
    requestedNumeric = selected_action() == 5 ? 9 : lv_dropdown_get_selected(indexChoice);
    ui_nozzle_details_show();
}
void ui_nozzle_actions_show(void);
void ui_nozzle_details_show(void) {
    if (!bambuStatus.nozzle_count)
        return;
    struct ui_numeric_field fields[3];
    unsigned count = 0, selected = 0;
    numericOwner = xtouch_printer_active();
    for (unsigned i = 0; i < bambuStatus.nozzle_count && i < 2; i++)
        if (bambuStatus.nozzle_valid & (1U << i)) {
            if (i == bambuStatus.active_nozzle)
                selected = count;
            numericIndices[count] = i;
            fields[count++] =
                (struct ui_numeric_field){i ? "T1" : "T0", bambuStatus.nozzle_target[i], 0, 300};
        }
    if (xtouch_bblp_chamber_heater() && bambuStatus.chamber_target_valid) {
        numericIndices[count] = 9;
        fields[count++] = (struct ui_numeric_field){xtouch_local("仓温", "Chamber"),
                                                    bambuStatus.chamber_target_temper, 0,
                                                    xtouch_bblp_chamber_max()};
    }
    for (unsigned i = 0; i < count; i++)
        if (numericIndices[i] == requestedNumeric)
            selected = i;
    requestedNumeric = 255;
    if (count)
        ui_numeric_show(fields, count, selected, numeric_done, NULL, ui_nozzle_actions_show);
}
static void action_changed(lv_event_t *e) {
    if (selected_action() == 5)
        lv_obj_add_state(indexChoice, LV_STATE_DISABLED);
    else
        lv_obj_clear_state(indexChoice, LV_STATE_DISABLED);
}
static void update_menu(void) {
    unsigned flags = (bambuStatus.nozzle_count > 1 ? 1 : 0) | (bambuStatus.rack_present ? 2 : 0) |
                     (xtouch_bblp_chamber_heater() ? 4 : 0);
    if (flags == menuFlags)
        return;
    unsigned old = actionCount ? selected_action() : 0;
    menuFlags = flags;
    actionCount = 0;
    char menu[256] = {0};
    size_t used = 0;
    unsigned selection = 0;
    const char *en[] = {"Set 0-300℃",    "Switch nozzle", "Extrude +10mm",
                        "Retract -10mm", "Hotend rack",   "Chamber 0-65℃"};
    const char *zh[] = {"设温 0-300℃", "切换喷头", "挤出 +10mm",
                        "回抽 -10mm",  "热端仓",   "仓温 0-65℃"};
    if (!strcmp(bambuStatus.printer_type, "C13")) {
        en[5] = "Chamber 0-60℃";
        zh[5] = "仓温 0-60℃";
    }
    for (unsigned a = 0; a < 6; a++) {
        if ((a == 1 && !(flags & 1)) || (a == 4 && !(flags & 2)) || (a == 5 && !(flags & 4)))
            continue;
        if (old == a)
            selection = actionCount;
        actionMap[actionCount++] = a;
        used += snprintf(menu + used, sizeof(menu) - used, "%s%s", used ? "\n" : "",
                         xtouch_local(zh[a], en[a]));
    }
    lv_dropdown_set_options(actionChoice, menu);
    lv_dropdown_set_selected(actionChoice, selection);
    action_changed(NULL);
}
static void refresh(lv_event_t *e) {
    if (printer != xtouch_printer_active()) {
        ui_detail_page_end(false);
        return;
    }
    update_menu();
    char text[160], v[2][32];
    for (unsigned i = 0; i < 2; i++) {
        if (bambuStatus.nozzle_valid & (1U << i))
            snprintf(v[i], sizeof(v[i]), "%u / %u ℃", bambuStatus.nozzle_actual[i],
                     bambuStatus.nozzle_target[i]);
        else
            snprintf(v[i], sizeof(v[i]), "--");
    }
    snprintf(text, sizeof(text), "T0 %s %s\nT1 %s %s\n%s",
             bambuStatus.active_nozzle == 0 ? "*" : " ", v[0],
             bambuStatus.active_nozzle == 1 ? "*" : " ", v[1],
             xtouch_local("* 当前喷头", "* active nozzle"));
    lv_label_set_text(readings, text);
    unsigned action = selected_action();
    struct ui_nozzle_request r = {printer, lv_dropdown_get_selected(indexChoice), 0,
                                  action == 5 ? 9 : action};
    bool allowed = action == 4 ? bambuStatus.rack_present : ui_nozzle_request_allowed(&r);
    if (allowed)
        lv_obj_clear_state(applyButton, LV_STATE_DISABLED);
    else
        lv_obj_add_state(applyButton, LV_STATE_DISABLED);
}
static void apply(lv_event_t *e) {
    unsigned action = selected_action();
    if (action == 0 || action == 5) {
        edit_event(NULL);
        return;
    }
    if (action == 4) {
        ui_hotend_details_show();
        return;
    }
    pending = (struct ui_nozzle_request){printer, lv_dropdown_get_selected(indexChoice), 0,
                                         action == 5 ? 9 : action};
    if (!ui_nozzle_request_allowed(&pending)) {
        ui_notice_unavailable();
        return;
    }
    char text[120];
    if (!action)
        snprintf(text, sizeof(text), "T%u  %u℃ ?", pending.index, pending.target);
    else if (action == 5)
        snprintf(text, sizeof(text), "%s %u℃?", xtouch_local("仓温", "Chamber"),
                 pending.target);
    else
        snprintf(text, sizeof(text), "T%u %s?", pending.index,
                 action == 1   ? (xtouch_local("切换喷头", "Switch nozzle"))
                 : action == 2 ? (xtouch_local("挤出 +10mm", "Extrude +10mm"))
                               : (xtouch_local("回抽 -10mm", "Retract -10mm")));
    ui_confirmPanel_show(text, confirm);
}
void ui_nozzle_actions_show(void) {
    if (!bambuStatus.nozzle_count)
        return;
    ui_detail_page_end(false);
    printer = xtouch_printer_active();
    dialog = ui_detail_page_begin();
    if (!dialog)
        return;
    menuFlags = 255;
    actionCount = 0;
    lv_obj_add_event_cb(dialog, deleted, LV_EVENT_DELETE, NULL);
    label(dialog, xtouch_local("喷头控制", "Nozzle control"), 2, 0);
    readings = label(dialog, "", 4, 33);
    lv_obj_set_style_text_line_space(readings, 6, 0);
    indexChoice = lv_dropdown_create(dialog);
    lv_obj_set_pos(indexChoice, 4, 132);
    lv_obj_set_size(indexChoice, 280, 34);
    lv_dropdown_set_options(indexChoice, bambuStatus.nozzle_count == 2 ? "T0\nT1" : "T0");
    if (bambuStatus.active_nozzle < bambuStatus.nozzle_count)
        lv_dropdown_set_selected(indexChoice, bambuStatus.active_nozzle);
    actionChoice = lv_dropdown_create(dialog);
    lv_obj_set_pos(actionChoice, 4, 180);
    lv_obj_set_size(actionChoice, 185, 34);
    update_menu();
    applyButton = lv_btn_create(dialog);
    lv_obj_set_size(applyButton, 84, 34);
    lv_obj_set_pos(applyButton, 200, 180);
    lv_obj_center(label(applyButton, xtouch_local("确认", "Confirm"), 0, 0));
    lv_obj_add_event_cb(applyButton, apply, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(indexChoice, refresh, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(actionChoice, action_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(actionChoice, refresh, LV_EVENT_VALUE_CHANGED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_NOZZLE_TEMP, dialog, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_MQTT, dialog, NULL);
    lv_obj_add_event_cb(dialog, refresh, LV_EVENT_MSG_RECEIVED, NULL);
    refresh(NULL);
}
