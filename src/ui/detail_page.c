#include "detail_page.h"
#include "chrome_metrics.h"
/* Modal chrome uses native square controls, not independently stretched axes. */
#undef lv_obj_set_size
#undef lv_obj_set_pos
#undef lv_obj_align
#undef lv_obj_set_style_pad_all
static lv_obj_t *active;
static int return_page;
/* Native content bounds are shared by every detail view, independent of assets. */
void ui_detail_page_title(lv_obj_t *page,lv_obj_t *title){
    lv_obj_update_layout(page);
    ui_chrome_metrics_t chrome=ui_chrome_metrics(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL));
    lv_obj_set_pos(title,0,0);
    lv_obj_set_size(title,LV_MAX(1,lv_obj_get_content_width(page)-chrome.control-chrome.edge),LV_SIZE_CONTENT);
    if(lv_label_get_long_mode(title)!=LV_LABEL_LONG_SCROLL)lv_label_set_long_mode(title,LV_LABEL_LONG_SCROLL);
    ui_text_guard(title);
    lv_area_t ink=ui_layout_ink(lv_obj_get_style_text_font(title,LV_PART_MAIN),lv_label_get_text(title));
    if(ink.y2>=ink.y1)lv_obj_set_pos(title,0,(chrome.control-lv_area_get_height(&ink))/2-ink.y1-lv_obj_get_style_pad_top(title,LV_PART_MAIN));
}
void ui_detail_page_block(lv_obj_t *page,lv_obj_t *child,int y,int height){
    lv_obj_update_layout(page);
    int scale=lv_disp_get_ver_res(NULL);
    lv_obj_set_pos(child,0,y*scale/240);
    lv_obj_set_size(child,lv_obj_get_content_width(page),height>0?height*scale/240:LV_SIZE_CONTENT);
}
void ui_detail_page_split(lv_obj_t *page,lv_obj_t *left,lv_obj_t *right,int y,int height,unsigned left_weight,unsigned right_weight){
    lv_obj_update_layout(page);
    ui_chrome_metrics_t chrome=ui_chrome_metrics(lv_disp_get_hor_res(NULL),lv_disp_get_ver_res(NULL));
    int width=lv_obj_get_content_width(page),gap=chrome.edge,scale=lv_disp_get_ver_res(NULL);
    unsigned total=left_weight+right_weight;if(!total)total=1;
    int first=(width-gap)*left_weight/total;
    lv_obj_set_pos(left,0,y*scale/240);lv_obj_set_size(left,first,height*scale/240);
    lv_obj_set_pos(right,first+gap,y*scale/240);lv_obj_set_size(right,width-first-gap,height*scale/240);
}
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
    const int width=lv_disp_get_hor_res(NULL),height=lv_disp_get_ver_res(NULL);
    const ui_chrome_metrics_t chrome=ui_chrome_metrics(width,height);
    active = lv_obj_create(lv_layer_top());
    lv_obj_set_size(active, width, height);
    lv_obj_set_pos(active, 0, 0);
    lv_obj_set_style_pad_all(active, chrome.edge, 0);
    lv_obj_set_style_border_width(active,0,0);
    lv_obj_set_style_radius(active,0,0);
    lv_obj_set_style_bg_color(active, lv_color_hex(0x333333), 0);
    lv_obj_clear_flag(active, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *close = lv_btn_create(active);
    lv_obj_set_size(close, chrome.control, chrome.control);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(close, lv_color_hex(0xAA2A00), 0);
    lv_obj_t *caption = lv_label_create(close);
    lv_label_set_text(caption, "X");
    ui_layout_close(close,caption,&ui_font_xlcdmin);

    lv_obj_center(caption);
    lv_obj_add_event_cb(close, close_page, LV_EVENT_CLICKED, NULL);
    lv_async_call(guard_contents,active);
    return active;
}
