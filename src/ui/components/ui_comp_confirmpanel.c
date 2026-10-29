#include "features/sound/events.h"
#include "../ui.h"
#include <string.h>
#include <stdio.h>

// COMPONENT confirmPanel

void (*ui_confirmPanel_showOnYes)();
static lv_timer_t *noticeTimer;
static lv_obj_t *noticeBox,*noticeLabel;
static ui_message_kind noticeKind;
static unsigned confirmationOwner;
static uint32_t confirmationAt;
void ui_notice_hide(void){if(noticeTimer){lv_timer_del(noticeTimer);noticeTimer=NULL;}if(noticeBox){lv_obj_del(noticeBox);noticeBox=noticeLabel=NULL;}}
bool ui_notice_visible(void){return noticeBox!=NULL;}
static void notice_timeout(lv_timer_t *timer){noticeTimer=NULL;ui_notice_hide();lv_timer_del(timer);printf("[NOTICE] close=auto\n");}
static void notice_clicked(lv_event_t *event){(void)event;ui_notice_hide();printf("[NOTICE] close=local\n");}

void ui_confirmPanel_NOOP(){};

static void compact_layout(void){
 lv_obj_t *caption=ui_comp_get_child(ui_confirmComponent,2),*panel=ui_comp_get_child(ui_confirmComponent,1),*buttons[]={ui_comp_get_child(ui_confirmComponent,3),ui_comp_get_child(ui_confirmComponent,5)};
 ui_layout_dialog(panel,lv_obj_get_parent(caption),buttons,2,false);
}
void ui_confirmPanel_show(const char *title, void (*onYES)(void))
{
    ui_notice_hide();
    ui_action_style(ui_comp_get_child(ui_confirmComponent,3),UI_ACTION_SECONDARY);
    ui_action_style(ui_comp_get_child(ui_confirmComponent,5),UI_ACTION_PRIMARY);
    lv_label_set_text(ui_comp_get_child(ui_confirmComponent,4),XTOUCH_TEXT_NO);
    lv_label_set_text(ui_comp_get_child(ui_confirmComponent,6),XTOUCH_TEXT_CONFIRM);
    lv_obj_t *confirmIcon=lv_obj_get_child(ui_comp_get_child(ui_confirmComponent,5),0);lv_label_set_text(confirmIcon,"q");
    lv_obj_clear_flag(ui_comp_get_child(ui_confirmComponent,3),LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_comp_get_child(ui_confirmComponent,5),LV_OBJ_FLAG_HIDDEN);
    ui_confirmPanel_showOnYes = onYES;
    confirmationOwner=xtouch_printer_active();confirmationAt=lv_tick_get();

    lv_obj_move_foreground(ui_confirmComponent);
    lv_obj_clear_flag(ui_confirmComponent, LV_OBJ_FLAG_HIDDEN); /// Flags
    lv_obj_t *titleLabel = ui_comp_get_child(ui_confirmComponent, UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER_CONFIRMPANELCAPTION);
    lv_label_set_text(titleLabel, title);
    ui_message_caption(titleLabel,UI_MESSAGE_CONFIRM);
    compact_layout();

    // [](int a){
}

void ui_confirmPanel_show_danger(const char *title,void (*onYES)(void)){ui_confirmPanel_show(title,onYES);ui_action_style(ui_comp_get_child(ui_confirmComponent,5),UI_ACTION_DESTRUCTIVE);}

bool ui_notice_show(const char *text,unsigned timeout){return ui_notice_show_kind(text,timeout,timeout?UI_MESSAGE_INFO:UI_MESSAGE_ERROR);}
bool ui_notice_show_kind(const char *text,unsigned timeout,ui_message_kind kind){
 if(!lv_obj_has_flag(ui_hmsComponent,LV_OBJ_FLAG_HIDDEN))return false;
 if(timeout&&!lv_obj_has_flag(ui_confirmComponent,LV_OBJ_FLAG_HIDDEN))return false;
 if(timeout){
  timeout=10000;
  if(noticeBox){const char *body=strchr(lv_label_get_text(noticeLabel),'\n');if(body&&noticeKind==kind&&!strcmp(body+1,text))return true;}
 if(kind==UI_MESSAGE_WARNING||kind==UI_MESSAGE_ERROR)xtouch_sound_event(kind==UI_MESSAGE_ERROR?XTOUCH_SOUND_ERROR:XTOUCH_SOUND_WARNING,lv_tick_get());
  ui_notice_hide();noticeKind=kind;noticeBox=lv_obj_create(lv_layer_top());noticeLabel=lv_label_create(noticeBox);lv_label_set_text(noticeLabel,text);ui_message_caption(noticeLabel,kind);ui_layout_notice(noticeBox,noticeLabel);lv_obj_add_event_cb(noticeBox,notice_clicked,LV_EVENT_CLICKED,NULL);noticeTimer=lv_timer_create(notice_timeout,timeout,NULL);printf("[NOTICE] show timeout=%u\n",timeout);return true;
 }
 if(kind==UI_MESSAGE_WARNING||kind==UI_MESSAGE_ERROR)xtouch_sound_event(kind==UI_MESSAGE_ERROR?XTOUCH_SOUND_ERROR:XTOUCH_SOUND_WARNING,lv_tick_get());
 ui_confirmPanel_show(text,ui_confirmPanel_NOOP);lv_obj_add_flag(ui_comp_get_child(ui_confirmComponent,3),LV_OBJ_FLAG_HIDDEN);
 lv_obj_t *caption=ui_comp_get_child(ui_confirmComponent,2);lv_label_set_text(caption,text);ui_message_caption(caption,kind);ui_action_style(ui_comp_get_child(ui_confirmComponent,5),UI_ACTION_SECONDARY);lv_label_set_text(ui_comp_get_child(ui_confirmComponent,6),xtouch_local("关闭","Close"));lv_label_set_text(lv_obj_get_child(ui_comp_get_child(ui_confirmComponent,5),0),"r");compact_layout();return true;
}

void ui_notice_unavailable(void){ui_notice_show_kind(xtouch_local("设备忙或状态已变更", "Device busy or state changed."),1800,UI_MESSAGE_WARNING);}

void ui_confirmPanel_state_changed(void){if(ui_confirmPanel_showOnYes&&ui_confirmPanel_showOnYes!=ui_confirmPanel_NOOP)ui_confirmPanel_hide();}

void ui_confirmPanel_hide()
{
    ui_notice_hide();
    ui_confirmPanel_showOnYes = NULL;
    lv_obj_add_flag(ui_confirmComponent, LV_OBJ_FLAG_HIDDEN); /// Flags
    // [](int a){
}

void ui_event_comp_confirmPanel_confirmPanelNO(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target_obj(e);
    lv_obj_t **comp_confirmPanel = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        ui_confirmPanel_hide();
    }
}
void ui_event_comp_confirmPanel_confirmPanelYES(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target_obj(e);
    lv_obj_t **comp_confirmPanel = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        /* Close this prompt first: its action may open a result/progress prompt. */
        void (*action)(void)=ui_confirmPanel_showOnYes;
        ui_confirmPanel_hide();
        if(action&&action!=ui_confirmPanel_NOOP&&(confirmationOwner!=xtouch_printer_active()||xtouch_printer_switch_pending()||lv_tick_elaps(confirmationAt)>30000)){
            ui_notice_unavailable();return;
        }
        if(action)action();
    }
}

lv_obj_t *ui_confirmPanel_create(lv_obj_t *comp_parent)
{

    lv_obj_t *cui_confirmPanel;
    cui_confirmPanel = lv_obj_create(comp_parent);
    lv_obj_set_width(cui_confirmPanel, lv_pct(100));
    lv_obj_set_height(cui_confirmPanel, lv_pct(100));
    lv_obj_set_flex_flow(cui_confirmPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_confirmPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(cui_confirmPanel, LV_OBJ_FLAG_FLOATING);                                                                                                                                                                                                       /// Flags
    lv_obj_clear_flag(cui_confirmPanel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_confirmPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_confirmPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_confirmPanel, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_confirmPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_confirmPanelContainer;
    cui_confirmPanelContainer = lv_obj_create(cui_confirmPanel);
    lv_obj_set_width(cui_confirmPanelContainer, lv_pct(100));
    lv_obj_set_height(cui_confirmPanelContainer, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_confirmPanelContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_confirmPanelContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_confirmPanelContainer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanelContainer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_confirmPanelContainer, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_confirmPanelContainer, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_confirmPanelContainer, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_confirmPanelCaption;
    cui_confirmPanelCaption = lv_label_create(cui_confirmPanelContainer);
    lv_obj_set_width(cui_confirmPanelCaption, lv_pct(100));      /// 1
    lv_obj_set_height(cui_confirmPanelCaption, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_confirmPanelCaption, XTOUCH_TEXT_ARE_YOU_SURE);
    lv_obj_clear_flag(cui_confirmPanelCaption, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanelCaption, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_left(cui_confirmPanelCaption, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_confirmPanelCaption, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_confirmPanelCaption, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_confirmPanelCaption, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_confirmPanelNO;
    cui_confirmPanelNO = lv_label_create(cui_confirmPanelContainer);
    lv_obj_set_height(cui_confirmPanelNO, LV_SIZE_CONTENT); /// 1
    lv_obj_set_flex_grow(cui_confirmPanelNO, 1);
    lv_obj_set_flex_flow(cui_confirmPanelNO, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_confirmPanelNO, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(cui_confirmPanelNO, "r");
    lv_obj_add_flag(cui_confirmPanelNO, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_confirmPanelNO, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanelNO, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_confirmPanelNO, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_confirmPanelNO, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_confirmPanelNO, lv_color_hex(0xAA2A00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_confirmPanelNO, lv_color_hex(0x552A00), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_confirmPanelNO, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_confirmPanelNO, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_confirmPanelNO, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_confirmPanelNO, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_confirmPanelNO, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_confirmPanelNOLabel;
    cui_confirmPanelNOLabel = lv_label_create(cui_confirmPanelNO);
    lv_obj_set_width(cui_confirmPanelNOLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_confirmPanelNOLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_confirmPanelNOLabel, XTOUCH_TEXT_NO);
    lv_obj_clear_flag(cui_confirmPanelNOLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanelNOLabel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_confirmPanelNOLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_confirmPanelYES;
    cui_confirmPanelYES = lv_label_create(cui_confirmPanelContainer);
    lv_obj_set_height(cui_confirmPanelYES, LV_SIZE_CONTENT); /// 1
    lv_obj_set_flex_grow(cui_confirmPanelYES, 1);
    lv_obj_set_flex_flow(cui_confirmPanelYES, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_confirmPanelYES, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(cui_confirmPanelYES, "q");
    lv_obj_add_flag(cui_confirmPanelYES, LV_OBJ_FLAG_CLICKABLE);                                                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_confirmPanelYES, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanelYES, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_confirmPanelYES, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_confirmPanelYES, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_confirmPanelYES, lv_color_hex(0x2AAA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_confirmPanelYES, lv_color_hex(0x2A5500), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_confirmPanelYES, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_confirmPanelYES, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_confirmPanelYES, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_confirmPanelYES, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_confirmPanelYES, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_confirmPanelYESLabel;
    cui_confirmPanelYESLabel = lv_label_create(cui_confirmPanelYES);
    lv_obj_set_width(cui_confirmPanelYESLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_confirmPanelYESLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_confirmPanelYESLabel, XTOUCH_TEXT_CONFIRM);
    lv_obj_clear_flag(cui_confirmPanelYESLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_confirmPanelYESLabel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_confirmPanelYESLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t **children = lv_mem_alloc(sizeof(lv_obj_t *) * _UI_COMP_CONFIRMPANEL_NUM);
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANEL] = cui_confirmPanel;
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER] = cui_confirmPanelContainer;
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER_CONFIRMPANELCAPTION] = cui_confirmPanelCaption;
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER_CONFIRMPANELNO] = cui_confirmPanelNO;
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER_CONFIRMPANELNO1_CONFIRMPANELNOLABEL] = cui_confirmPanelNOLabel;
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER_CONFIRMPANELYES] = cui_confirmPanelYES;
    children[UI_COMP_CONFIRMPANEL_CONFIRMPANELCONTAINER_CONFIRMPANELYES_CONFIRMPANELYESLABEL] = cui_confirmPanelYESLabel;
    lv_obj_add_event_cb(cui_confirmPanel, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children);
    lv_obj_add_event_cb(cui_confirmPanel, del_component_child_event_cb, LV_EVENT_DELETE, children);
    lv_obj_add_event_cb(cui_confirmPanelNO, ui_event_comp_confirmPanel_confirmPanelNO, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_confirmPanelYES, ui_event_comp_confirmPanel_confirmPanelYES, LV_EVENT_ALL, children);
    lv_obj_t *body=lv_obj_create(cui_confirmPanelContainer);lv_obj_remove_style_all(body);lv_obj_set_scroll_dir(body,LV_DIR_VER);lv_obj_set_scrollbar_mode(body,LV_SCROLLBAR_MODE_AUTO);lv_obj_clear_flag(body,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_parent(cui_confirmPanelCaption,body);lv_obj_set_width(cui_confirmPanelCaption,lv_pct(100));lv_obj_set_style_pad_all(cui_confirmPanelCaption,0,0);lv_obj_set_style_text_align(cui_confirmPanelCaption,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(cui_confirmPanelCaption,LV_LABEL_LONG_WRAP);
    ui_layout_action(cui_confirmPanelNO,&ui_font_xlcdmin,"r");
    ui_layout_action(cui_confirmPanelYES,&ui_font_xlcdmin,"q");
    lv_obj_t *actions[]={cui_confirmPanelNO,cui_confirmPanelYES};ui_layout_dialog(cui_confirmPanelContainer,body,actions,2,false);
    ui_comp_confirmPanel_create_hook(cui_confirmPanel);
    return cui_confirmPanel;
}
