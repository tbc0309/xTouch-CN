#include "home_clock.h"
#include "xtouch/clock.h"
#include <string.h>
// Compare actual font pixels with the actual flexible gap on every display.
#ifdef lv_txt_get_size
#undef lv_txt_get_size
#endif
LV_FONT_DECLARE(ui_font_text_14);

static void refresh(lv_timer_t *timer)
{
    lv_obj_t *gap = timer->user_data;
    lv_obj_t *label = lv_obj_get_child(gap, 0);
    char text[6];
    bool visible = xtouch_clock_text(text, sizeof(text));
    if (visible) {
        if (strcmp(lv_label_get_text(label), text)) lv_label_set_text(label, text);
        lv_point_t size;
        lv_txt_get_size(&size, text, lv_obj_get_style_text_font(label, 0),
                        lv_obj_get_style_text_letter_space(label, 0), 0,
                        LV_COORD_MAX, LV_TEXT_FLAG_EXPAND);
        // No layout participation: showing the clock never squeezes the AMS.
        visible = lv_obj_get_content_width(gap) >= size.x + 8;
    }
    if (visible) lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
}
static void resized(lv_event_t *event) { refresh(lv_event_get_user_data(event)); }
static void deleted(lv_event_t *event) { lv_timer_del(lv_event_get_user_data(event)); }
static void tick(lv_timer_t *timer)
{
    lv_obj_update_layout(timer->user_data);
    refresh(timer);
}
void ui_home_clock_bind(lv_obj_t *gap)
{
    lv_obj_t *label = lv_label_create(gap);
    lv_label_set_text(label, "00:00");
    lv_obj_add_flag(label, LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(label, &ui_font_text_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_timer_t *timer = lv_timer_create(tick, 1000, gap);
    if (!timer) return;
    lv_obj_add_event_cb(gap, resized, LV_EVENT_SIZE_CHANGED, timer);
    lv_obj_add_event_cb(gap, deleted, LV_EVENT_DELETE, timer);
    tick(timer);
}
