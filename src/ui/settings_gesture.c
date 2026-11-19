#include "settings_gesture.h"
// Gesture thresholds use physical display pixels, not design coordinates.
#ifdef lv_indev_get_point
#undef lv_indev_get_point
#endif

struct ui_scroll_guard {
    lv_obj_t *page, *target;
    lv_point_t origin;
    uint32_t last_scroll;
    bool scrolled, scrolling, blocked, horizontal;
    struct ui_scroll_guard *next;
};
typedef ui_scroll_guard_t settings_gesture;
static settings_gesture *pages;

static void control_event(lv_event_t *e);
static void unbind_controls(lv_obj_t *obj, settings_gesture *g) {
    lv_obj_remove_event_cb_with_user_data(obj, control_event, g);
    for (unsigned i = 0; i < lv_obj_get_child_cnt(obj); ++i) unbind_controls(lv_obj_get_child(obj, i), g);
}
static void page_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_DELETE && code != LV_EVENT_SCROLL_BEGIN && code != LV_EVENT_SCROLL && code != LV_EVENT_SCROLL_END) return;
    settings_gesture *g = lv_event_get_user_data(e);
    if (lv_event_get_target_obj(e) != g->page) return;
    if (code == LV_EVENT_DELETE) {
        settings_gesture **link=&pages;
        while(*link&&*link!=g)link=&(*link)->next;
        if(*link)*link=g->next;
        unbind_controls(g->page, g);
        lv_obj_remove_event_cb_with_user_data(g->page, page_event, g);
        lv_mem_free(g); return;
    }
    if (code == LV_EVENT_SCROLL_BEGIN || code == LV_EVENT_SCROLL || code == LV_EVENT_SCROLL_END) {
        g->scrolled = true;
        g->last_scroll = lv_tick_get();
        if (code == LV_EVENT_SCROLL_BEGIN) g->scrolling = true;
        if (code == LV_EVENT_SCROLL_END) g->scrolling = false;
        g->blocked = true;
    }
}
static void control_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING && code != LV_EVENT_RELEASED &&
        code != LV_EVENT_CLICKED && code != LV_EVENT_SHORT_CLICKED &&
        code != LV_EVENT_LONG_PRESSED && code != LV_EVENT_LONG_PRESSED_REPEAT) return;
    lv_indev_t *input = lv_indev_get_act();
    if (!input || lv_indev_get_type(input) != LV_INDEV_TYPE_POINTER) return;
    lv_obj_t *target = lv_event_get_target_obj(e);
    if (target != lv_event_get_current_target_obj(e)) return;
    settings_gesture *g = lv_event_get_user_data(e);
    bool slider = lv_obj_check_type(target, &lv_slider_class);
    if (code == LV_EVENT_PRESSED) {
        g->target = target;
        lv_indev_get_point(input, &g->origin);
        g->horizontal = false;
        g->blocked = g->scrolling || (g->scrolled && lv_tick_elaps(g->last_scroll) < 250);
    }
    if (g->target != target) return;
    if (code == LV_EVENT_PRESSING && !g->blocked && !g->horizontal) {
        lv_point_t point; lv_indev_get_point(input, &point);
        int dx = LV_ABS(point.x - g->origin.x), dy = LV_ABS(point.y - g->origin.y);
        int horizontal = LV_MAX(8, lv_disp_get_hor_res(NULL) / 40);
        int vertical = LV_MAX(8, lv_disp_get_ver_res(NULL) / 30);
        if (slider) {
            if (dy >= vertical && dy >= dx) g->blocked = true;
            else if (dx >= horizontal && dx > dy * 2) g->horizontal = true;
        } else if (dx >= horizontal || dy >= vertical) g->blocked = true;
    }
    /* LVGL 9 also moves the knob from RELEASED, even without a drag.
     * An undecided tap or vertical swipe must never commit a slider value. */
    if (slider && code == LV_EVENT_RELEASED && !g->horizontal) {
        lv_event_stop_processing(e); return;
    }
    if (g->blocked) {
        // Let the widget clean up its pressed/drag state without releasing,
        // checking a switch, dispatching a click or saving a slider value.
        if (code == LV_EVENT_RELEASED) lv_obj_send_event(target, LV_EVENT_PRESS_LOST, input);
        lv_event_stop_processing(e);
    } else if (slider && code == LV_EVENT_PRESSING && !g->horizontal) {
        // Do not jump to the initial finger position before direction is known.
        lv_event_stop_processing(e);
    }
}
static void bind_controls(lv_obj_t *obj, settings_gesture *g)
{
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_add_event_cb(obj, control_event, LV_EVENT_ALL | LV_EVENT_PREPROCESS, g);
    }
    for (unsigned i = 0; i < lv_obj_get_child_cnt(obj); ++i) bind_controls(lv_obj_get_child(obj, i), g);
}
ui_scroll_guard_t *ui_scroll_guard_create(lv_obj_t *page)
{
    settings_gesture *g = lv_mem_alloc(sizeof(*g));
    if (!g) return NULL;
    *g = (settings_gesture){.page = page,.next=pages};pages=g;
    lv_obj_add_event_cb(page, page_event, LV_EVENT_ALL, g);
    return g;
}
void ui_scroll_guard_add(ui_scroll_guard_t *guard,lv_obj_t *control)
{
    if(guard&&control)bind_controls(control,guard);
}
bool ui_scroll_busy(uint32_t quiet_ms)
{
    for(settings_gesture *g=pages;g;g=g->next)
        if(lv_obj_is_visible(g->page)&&(g->scrolling||lv_obj_is_scrolling(g->page)||
           (g->scrolled&&lv_tick_elaps(g->last_scroll)<quiet_ms)))return true;
    return false;
}
void ui_settings_gesture_bind(lv_obj_t *page)
{
    settings_gesture *g=ui_scroll_guard_create(page);
    if(!g)return;
    for(unsigned i=0;i<lv_obj_get_child_cnt(page);++i)bind_controls(lv_obj_get_child(page,i),g);
}
