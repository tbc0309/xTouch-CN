#include "detail_page.h"
#include "i18n/language.h"
#include "nozzle_details.h"
#include <stdio.h>
static lv_obj_t *page, *summary, *choice, *operation, *submit;
static unsigned owner, ids[16], count;
static struct ui_nozzle_request request;
static const unsigned actions[] = {4, 4, 5, 5, 6, 7, 8};
static void accepted(void) {
    if (ui_nozzle_request_allowed(&request))
        lv_msg_send(XTOUCH_COMMAND_NOZZLE_INDEXED, &request);
}
static void refresh(lv_event_t *e) {
    if (owner != xtouch_printer_active()) {
        ui_detail_page_end(false);
        return;
    }
    char text[100];
    snprintf(text, sizeof(text), "%s %u   %s %u\n%s %u", xtouch_local("状态", "Status"),
             bambuStatus.rack_status, xtouch_local("位置", "Position"),
             bambuStatus.rack_position, xtouch_local("读取中", "Reading"),
             bambuStatus.rack_reading);
    lv_label_set_text(summary, text);
    unsigned op = lv_dropdown_get_selected(operation), selected = lv_dropdown_get_selected(choice);
    struct ui_nozzle_request r = {owner, 0,
                                  op == 1 || op == 3 ? 255
                                  : selected < count ? ids[selected]
                                                     : 255,
                                  op < 7 ? actions[op] : 99};
    if (ui_nozzle_request_allowed(&r))
        lv_obj_clear_state(submit, LV_STATE_DISABLED);
    else
        lv_obj_add_state(submit, LV_STATE_DISABLED);
}
static void apply(lv_event_t *e) {
    unsigned op = lv_dropdown_get_selected(operation), selected = lv_dropdown_get_selected(choice);
    if (op >= 7)
        return;
    request = (struct ui_nozzle_request){owner, 0,
                                         op == 1 || op == 3 ? 255
                                         : selected < count ? ids[selected]
                                                            : 255,
                                         actions[op]};
    if (!ui_nozzle_request_allowed(&request))
        return;
    char text[160], name[60];
    lv_dropdown_get_selected_str(operation, name, sizeof(name));
    snprintf(text, sizeof(text), "%s?\n%s", name,
             xtouch_local("打印头和热端仓可能移动", "Toolhead / rack may move."));
    ui_confirmPanel_show(text, accepted);
}
static lv_obj_t *label(lv_obj_t *p, const char *text, int x, int y) {
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, text);
    lv_obj_set_pos(l, x, y);
    return l;
}
void ui_hotend_details_show(void) {
    if (!bambuStatus.rack_present)
        return;
    ui_detail_page_end(false);
    owner = xtouch_printer_active();
    page = ui_detail_page_begin();
    if (!page)
        return;
    ui_detail_page_title(page,label(page, xtouch_local("H2C 热端仓", "H2C hotend rack"), 2, 0));
    summary = label(page, "", 4, 33);
    lv_obj_set_style_text_line_space(summary, 5, 0);
    char options[600] = {0};
    size_t used = 0;
    count = 0;
    for (unsigned i = 0; i < 16; i++)
        if (bambuStatus.rack_mask & (1U << i)) {
            ids[count++] = i;
            used += snprintf(options + used, sizeof(options) - used, "%sH%u · %.1f mm · %s",
                             used ? "\n" : "", i + 1, bambuStatus.rack_diameter[i] / 10.0,
                             bambuStatus.rack_type[i]);
        }
    choice = lv_dropdown_create(page);
    lv_dropdown_set_options(choice, count ? options : "--");
    operation = lv_dropdown_create(page);
    lv_dropdown_set_options(
        operation,
        xtouch_local("重读当前热端\n重读全部热端\n确认当前热端\n确认全部热端\n热端仓归位\n升起 A\n升起 B", "Read selected\nRead all\nConfirm selected\nConfirm all\nHome rack\nRaise A\nRaise B"));
    submit = lv_btn_create(page);
    label(submit, xtouch_local("确认", "Confirm"), 0, 0);
    ui_detail_page_block(page,summary,33,0);
    ui_detail_page_block(page,choice,90,32);
    ui_detail_page_block(page,operation,132,32);
    ui_detail_page_block(page,submit,180,34);
    ui_layout_dropdown(choice);ui_layout_dropdown(operation);
    lv_obj_add_event_cb(submit, apply, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(choice, refresh, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(operation, refresh, LV_EVENT_VALUE_CHANGED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_NOZZLE_TEMP, page, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_MQTT, page, NULL);
    lv_obj_add_event_cb(page, refresh, LV_EVENT_MSG_RECEIVED, NULL);
    refresh(NULL);
    ui_layout_captions_tree(page);
}
