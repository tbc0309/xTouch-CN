#include "numeric_editor.h"
#include "detail_page.h"
#include <stdio.h>
#include <stdlib.h>
extern lv_obj_t *ui_temperatureComponent_input_target;
extern int ui_temperatureComponent_backToScreen;
/* Reuse the original keyboard for all new numeric controls. */
static bool numericActive;
static unsigned numericOwner, numericCount, numericSelected;
static int numericBack;
static struct ui_numeric_field numericFields[4];
static char numericNames[4][32];
static lv_obj_t *numericTabs[4];
static ui_numeric_apply numericApply;
static void (*numericCancel)(void), (*numericMore)(void);
static void numeric_deleted(lv_event_t *e) {
    numericActive = false;
    ui_temperatureComponent_input_target = NULL;
}
static void numeric_close(void) {
    void (*back)(void) = numericCancel;
    numericActive = false;
    if (back)
        back();
    else
        loadScreen(numericBack);
}
static void numeric_tab(lv_event_t *e) {
    unsigned index = (uintptr_t)lv_event_get_user_data(e);
    numericSelected = index;
    for (unsigned i = 0; i < numericCount; i++) {
        if (i == index)
            lv_obj_add_state(numericTabs[i], LV_STATE_CHECKED);
        else
            lv_obj_clear_state(numericTabs[i], LV_STATE_CHECKED);
    }
    char text[32];
    snprintf(text, sizeof(text), "%u", numericFields[index].value);
    lv_textarea_set_placeholder_text(ui_temperatureComponent_input_target, text);
    lv_textarea_set_text(ui_temperatureComponent_input_target, "");
    lv_label_set_text(
        ui_comp_get_child(ui_temperatureComponent,
                          UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTKEYBOARD_NUMPADKEYBACK),
        LV_SYMBOL_CLOSE);
}
static void numeric_more(lv_event_t *e) {
    if (numericMore)
        numericMore();
}
void ui_numeric_show(const struct ui_numeric_field *fields, unsigned count, unsigned selected,
                     ui_numeric_apply apply, void (*cancel)(void), void (*more)(void)) {
    if (!fields || !count || count > 4 || !apply)
        return;
    int back = xTouchConfig.currentScreenIndex;
    ui_detail_page_end(false);
    loadScreen(1);
    ui_temperatureComponent_backToScreen = -1;
    numericBack = back >= 0 ? back : 1;
    numericOwner = xtouch_printer_active();
    numericCount = count;
    numericApply = apply;
    numericCancel = cancel;
    numericMore = more;
    for (unsigned i = 0; i < count; i++) {
        numericFields[i] = fields[i];
        snprintf(numericNames[i], sizeof(numericNames[i]), "%s", fields[i].name);
        numericFields[i].name = numericNames[i];
    }
    lv_obj_del(ui_comp_get_child(ui_temperatureComponent,
                                 UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTTEMPS));
    lv_obj_del(ui_comp_get_child(ui_temperatureComponent,
                                 UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTFANS));
    lv_obj_add_flag(ui_sidebarComponent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *left = lv_obj_create(ui_temperatureComponent);
    lv_obj_set_size(left, 96, lv_pct(100));
    lv_obj_set_style_pad_all(left, 2, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(left, 0);
    for (unsigned i = 0; i < count; i++) {
        lv_obj_t *b = lv_btn_create(left);
        numericTabs[i] = b;
        bool compact = count == 4 && more;
        lv_obj_set_pos(b, 0, i * (compact ? 29 : 38));
        lv_obj_set_size(b, 90, compact ? 27 : 34);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x555555), 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x247AA8), LV_STATE_CHECKED);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, numericNames[i]);
        lv_obj_center(l);
        lv_obj_add_event_cb(b, numeric_tab, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
    ui_temperatureComponent_input_target = lv_textarea_create(left);
    lv_obj_set_pos(ui_temperatureComponent_input_target, 0, more ? 120 : 176);
    lv_obj_set_size(ui_temperatureComponent_input_target, 90, 48);
    lv_textarea_set_one_line(ui_temperatureComponent_input_target, true);
    lv_textarea_set_max_length(ui_temperatureComponent_input_target, 3);
    lv_textarea_set_accepted_chars(ui_temperatureComponent_input_target, "0123456789");
    lv_obj_set_style_text_font(ui_temperatureComponent_input_target, XTOUCH_FONT_NUMERIC, 0);
    lv_obj_set_style_pad_all(ui_temperatureComponent_input_target, 4, 0);
    lv_obj_set_style_text_color(ui_temperatureComponent_input_target, lv_color_hex(0xFFAA00), 0);
    lv_obj_add_event_cb(ui_temperatureComponent_input_target,ui_layout_numeric_center_event,LV_EVENT_SIZE_CHANGED,NULL);
    lv_obj_add_event_cb(ui_temperatureComponent_input_target,ui_layout_numeric_center_event,LV_EVENT_VALUE_CHANGED,NULL);

    if (more) {
        lv_obj_t *b = lv_btn_create(left);
        lv_obj_set_pos(b, 0, 170);
        lv_obj_set_size(b, 90, 52);
        lv_obj_set_style_pad_all(b, 0, 0);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, xtouch_local("更多", "More"));
        lv_obj_center(l);
        lv_obj_add_event_cb(b, numeric_more, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t *keyboard = ui_comp_get_child(
        ui_temperatureComponent, UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTKEYBOARD);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_grow(keyboard, 1);
    numericActive = true;
    lv_obj_add_event_cb(ui_temperatureComponent, numeric_deleted, LV_EVENT_DELETE, NULL);
    numericSelected = selected < count ? selected : 0;
    for (unsigned i = 0; i < count; i++)
        if (i == numericSelected)
            lv_obj_add_state(numericTabs[i], LV_STATE_CHECKED);
    char value[12];
    snprintf(value, sizeof(value), "%u", numericFields[numericSelected].value);
    lv_textarea_set_placeholder_text(ui_temperatureComponent_input_target, value);
    lv_label_set_text(
        ui_comp_get_child(ui_temperatureComponent,
                          UI_COMP_TEMPERATURECOMPONENT_TEMPERATURECOMPONENTKEYBOARD_NUMPADKEYBACK),
        LV_SYMBOL_CLOSE);
    ui_text_guard_tree(left);
}

bool ui_numeric_active(void) { return numericActive; }
void ui_numeric_cancel(void) { numeric_close(); }
void ui_numeric_commit(void) {
    const char *text = lv_textarea_get_text(ui_temperatureComponent_input_target);
    if (!*text)
        return;
    unsigned value = atoi(text);
    struct ui_numeric_field *f = &numericFields[numericSelected];
    if (value < f->min || value > f->max) {
        char range[32];
        snprintf(range, sizeof(range), "%u - %u", f->min, f->max);
        lv_textarea_set_text(ui_temperatureComponent_input_target, "");
        lv_textarea_set_placeholder_text(ui_temperatureComponent_input_target, range);
        return;
    }
    if (numericOwner != xtouch_printer_active() || xtouch_printer_switch_pending()) {
        numeric_close();
        return;
    }
    numericApply(numericSelected, value);
    return;
}
