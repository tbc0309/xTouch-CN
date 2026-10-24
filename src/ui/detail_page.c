#include "detail_page.h"
static lv_obj_t *active;
static int return_page;
static void guard_contents(void *page){if(active==page)ui_text_guard_tree(active);}
void ui_detail_page_end(bool restore) {
    if (!active)
        return;
    lv_obj_t *old = active;
    active = NULL;
    lv_obj_del(old);
    if (restore)
        loadScreen(return_page);
}
void ui_detail_page_navigation(void) { ui_detail_page_end(false); }
static void close_page(lv_event_t *e) { ui_detail_page_end(true); }
lv_obj_t *ui_detail_page_begin(void) {
    int back = xTouchConfig.currentScreenIndex >= 0 ? xTouchConfig.currentScreenIndex : return_page;
    ui_detail_page_end(false);
    // Temperature has a full keypad. Never retain it under another full-screen UI.
    loadScreen(-1);
    return_page = back;
    lv_mem_monitor_t memory;
    lv_mem_monitor(&memory);
    if (memory.free_size < 8192 || memory.free_biggest_size < 2048) {
        loadScreen(back);
        return NULL;
    }
    active = lv_obj_create(lv_layer_top());
    lv_obj_set_size(active, 320, 240);
    lv_obj_set_pos(active, 0, 0);
    lv_obj_set_style_pad_all(active, 10, 0);
    lv_obj_set_style_bg_color(active, lv_color_hex(0x333333), 0);
    lv_obj_clear_flag(active, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *close = lv_btn_create(active);
    lv_obj_set_size(close, 34, 28);
    lv_obj_set_pos(close, 264, 0);
    lv_obj_set_style_bg_color(close, lv_color_hex(0x9D3434), 0);
    lv_obj_t *caption = lv_label_create(close);
    lv_label_set_text(caption, "X");
    ui_layout_close(close,caption,&ui_font_xlcdmin);

    lv_obj_center(caption);
    lv_obj_add_event_cb(close, close_page, LV_EVENT_CLICKED, NULL);
    lv_async_call(guard_contents,active);
    return active;
}
