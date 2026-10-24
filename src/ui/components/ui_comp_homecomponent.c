#include "features/ui_profile.h"
#if defined(XTOUCH_HAS_CAMERA) && XTOUCH_HAS_CAMERA
#include <lvgl.h>
#include "camera.h"
static void cameraClick(lv_event_t *event){(void)event;if(xtouch_camera_available())xtouch_camera_open();}
#endif
#include <Arduino.h>
#include <stdio.h>
#include "../ui.h"
#include "../printer_models.h"
#include "../home_clock.h"

static lv_obj_t *printPreviewBox,*printPreviewImage,*printPreviewEmpty;
static const lv_img_dsc_t *printPreviewSource;
static bool printPreviewHasThumbnail;
static void print_preview_fit(lv_event_t *event){
    (void)event;
    if(!printPreviewImage)return;
    const lv_img_dsc_t *image=printPreviewSource;
    if(!image||!image->header.w||!image->header.h)return;
    int w=lv_obj_get_content_width(printPreviewBox),h=lv_obj_get_content_height(printPreviewBox);
    if(w<=0||h<=0)return;
    unsigned zoomX=(unsigned)w*256/image->header.w,zoomY=(unsigned)h*256/image->header.h;
    lv_img_set_zoom(printPreviewImage,image->header.cf==LV_IMG_CF_RAW?256:(zoomX<zoomY?zoomX:zoomY));
    lv_obj_center(printPreviewImage);
}
void ui_home_preview_bounds(uint16_t *width,uint16_t *height){
    *width=*height=0;if(!printPreviewBox)return;lv_obj_update_layout(printPreviewBox);
    int w=lv_obj_get_content_width(printPreviewBox),h=lv_obj_get_content_height(printPreviewBox);
    if(w>0&&h>0){*width=w;*height=h;}
}
void ui_home_preview_set(const lv_img_dsc_t *image){
    printPreviewHasThumbnail=image!=NULL;
#if !XTOUCH_HAS_PRINT_PREVIEW
    printPreviewHasThumbnail=false;
    image=ui_printer_model_image(xtouch_printer_model_at(xtouch_printer_active()));
#else
    if(!image)image=ui_printer_model_image(xtouch_printer_model_at(xtouch_printer_active()));
#endif
    if(!printPreviewImage)return;
    if(image&&image==printPreviewSource)return;
    printPreviewSource=image;
    if(!image){lv_img_set_src(printPreviewImage,LV_SYMBOL_IMAGE);lv_obj_add_flag(printPreviewImage,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(printPreviewEmpty,LV_OBJ_FLAG_HIDDEN);return;}
    lv_img_set_src(printPreviewImage,image);
    lv_obj_clear_flag(printPreviewImage,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(printPreviewEmpty,LV_OBJ_FLAG_HIDDEN);
    lv_obj_update_layout(printPreviewBox);print_preview_fit(NULL);
}
static void print_preview_deleted(lv_event_t *event){(void)event;printPreviewHasThumbnail=false;printPreviewSource=NULL;printPreviewBox=printPreviewImage=printPreviewEmpty=NULL;}
static void print_button_layout(lv_event_t *event){
    ui_layout_print_button(lv_event_get_target(event));
}
#include "../ams_display.h"
#include "../printer_picker.h"
#include "../printer_models.h"
LV_FONT_DECLARE(ui_font_text_24);
#include "xtouch/globals.h"

static void set_device_model(lv_obj_t *label)
{
    if(!printPreviewHasThumbnail)ui_home_preview_set(NULL);
    const char *name=ui_printer_model_short_name(xtouch_printer_model_at(xtouch_printer_active()));
    lv_label_set_text(label,name);
    lv_obj_t *row=lv_obj_get_parent(label);
    lv_obj_update_layout(row);
    ui_layout_device_model(row);
}
static void device_model_updated(lv_event_t *e){set_device_model(lv_event_get_target(e));}

static void on_next_printer(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
        ui_printer_picker_show();
        return;
    }
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) return;
    unsigned count=0;for(unsigned i=0;i<XTOUCH_MAX_PRINTERS;++i)count+=xtouch_printer_configured(i);
    if(count>=3){ui_printer_picker_show();return;}
    uint8_t current = xtouch_printer_active();
    for (uint8_t offset = 1; offset < XTOUCH_MAX_PRINTERS; ++offset) {
        uint8_t slot = (current + offset) % XTOUCH_MAX_PRINTERS;
        if (xtouch_printer_configured(slot)) { xtouch_printer_request(slot); return; }
    }
}

const char *xtouch_device_get_print_state()
{
    if (!xtouch_printer_configured(xtouch_printer_active())) return XTOUCH_TEXT_NO_PRINTER;
    if (!xtouch_printer_online()) return XTOUCH_TEXT_PRINTER_WAITING;
    switch (bambuStatus.print_status)
    {
    case XTOUCH_PRINT_STATUS_IDLE:
        return XTOUCH_TEXT_READY;
    case XTOUCH_PRINT_STATUS_FINISHED:
        return XTOUCH_TEXT_FINISHED;
    case XTOUCH_PRINT_STATUS_FAILED:
        return XTOUCH_TEXT_FAILED;
    case XTOUCH_PRINT_STATUS_RUNNING:
        return XTOUCH_TEXT_PRINTING;
    case XTOUCH_PRINT_STATUS_PAUSED:
        return XTOUCH_TEXT_PAUSED;
    case XTOUCH_PRINT_STATUS_PREPARE:
        return XTOUCH_TEXT_PREPARING;
    default:
        return "N/A";
    }
}

void ui_event_comp_homeComponent_mainScreenPlayPauseButton(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        onHomeControllerPlayPause(e);
    }
}
void ui_event_comp_homeComponent_mainScreenStopButton(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        onHomeControllerStop(e);
    }
}
void ui_event_comp_homeComponent_mainScreenSpeedDropDown(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        onHomeSpeedSelection(e);
    }
}
void ui_event_comp_homeComponent_mainScreenLightButton(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        onHomeLight(e);
        _ui_state_modify(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENLIGHTBUTTON], LV_STATE_CHECKED, _UI_MODIFY_STATE_TOGGLE);
    }
}
void ui_event_comp_homeComponent_mainScreenBedTemp(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        onHomeBedTemp(e);
    }
}
void ui_event_comp_homeComponent_mainScreenNozzleTemp(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        onHomeNozzleTemp(e, 0);
    }
}

void ui_event_comp_homeComponent_mainScreenSpeedChange(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    int last_speed = bambuStatus.printing_speed_lvl;
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        char buf[32];
        uint16_t index = lv_dropdown_get_selected(target);
        switch (index)
        {
        case 0:
            bambuStatus.printing_speed_lvl = XTOUCH_SPEED_LEVEL_SILENCE;
            break;
        case 1:
            bambuStatus.printing_speed_lvl = XTOUCH_SPEED_LEVEL_NORMAL;
            break;
        case 2:
            bambuStatus.printing_speed_lvl = XTOUCH_SPEED_LEVEL_RAPID;
            break;
        case 3:
            bambuStatus.printing_speed_lvl = XTOUCH_SPEED_LEVEL_RAMPAGE;
            break;
        }
        lv_msg_send(XTOUCH_COMMAND_PRINT_SPEED, NULL);
    }
}

void onXTouchLightMessage(lv_event_t *e)
{

    lv_obj_t *target = lv_event_get_target(e);
    lv_msg_t *m = lv_event_get_msg(e);

    struct XTOUCH_MESSAGE_DATA *message = (struct XTOUCH_MESSAGE_DATA *)m->payload;

    if (message->data == 1)
    {

        lv_obj_add_state(target, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_clear_state(target, LV_STATE_CHECKED);
    }
}

void onXTouchBedTemp(lv_event_t *e)
{

    lv_obj_t *target = lv_event_get_target(e);
    lv_msg_t *m = lv_event_get_msg(e);

    struct XTOUCH_MESSAGE_DATA *message = (struct XTOUCH_MESSAGE_DATA *)m->payload;

    char value[10];

    unsigned bit=lv_msg_get_id(m)==XTOUCH_ON_BED_TEMP?1:4;
    if(!(bambuStatus.temperature_valid&bit)){lv_label_set_text(target,"--");return;}
    itoa(message->data, value, 10);
    lv_label_set_text(target, value);
}

void onXTouchChamberTemp(lv_event_t *e)
{

    lv_obj_t *target = lv_event_get_target(e);
    lv_msg_t *m = lv_event_get_msg(e);

    struct XTOUCH_MESSAGE_DATA *message = (struct XTOUCH_MESSAGE_DATA *)m->payload;
    char value[10];

    if(!xtouch_bblp_chamber_display()){
        lv_obj_add_flag(lv_obj_get_parent(target),LV_OBJ_FLAG_HIDDEN);lv_label_set_text(target,"--");return;
    }
    lv_obj_clear_flag(lv_obj_get_parent(target),LV_OBJ_FLAG_HIDDEN);
    if(!bambuStatus.chamber_valid){lv_label_set_text(target,"--");return;}
    itoa(message->data, value, 10);
    lv_label_set_text(target, value);
}

void onXTouchBedTempTarget(lv_event_t *e)
{

    lv_obj_t *target = lv_event_get_target(e);
    lv_msg_t *m = lv_event_get_msg(e);

    struct XTOUCH_MESSAGE_DATA *message = (struct XTOUCH_MESSAGE_DATA *)m->payload;
    lv_obj_set_style_text_color(target, message->data > 0 ? lv_color_hex(0xff682a) : lv_color_hex(0xCCCCCC), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void onXTouchWifiSignal(lv_event_t *e)
{

    lv_obj_t *target = lv_event_get_target(e);
    lv_msg_t *m = lv_event_get_msg(e);

    struct XTOUCH_MESSAGE_DATA *message = (struct XTOUCH_MESSAGE_DATA *)m->payload;
    lv_obj_set_style_text_color(target, message->data < 50 ? lv_color_hex(0x2aff00) : lv_color_hex(0xff682a), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void printerSignal(lv_event_t *e){
    lv_obj_t *label=lv_event_get_target(e);
    char text[16];
    if(bambuStatus.wifi_signal_valid)snprintf(text,sizeof(text),"%d",-bambuStatus.wifi_signal);
    else snprintf(text,sizeof(text),"--");
    lv_label_set_text(label,text);
    ui_layout_status(lv_obj_get_parent(label));
}

void onXTouchIPCam(lv_event_t *e)
{

    lv_obj_t *target = lv_event_get_target(e);
    bool available=true;
#if defined(XTOUCH_HAS_CAMERA) && XTOUCH_HAS_CAMERA
    available=xtouch_camera_available();
    if(available)lv_obj_add_flag(lv_obj_get_parent(target),LV_OBJ_FLAG_CLICKABLE);
    else lv_obj_clear_flag(lv_obj_get_parent(target),LV_OBJ_FLAG_CLICKABLE);
#endif
    if (bambuStatus.has_ipcam && available)
    {
        lv_obj_clear_flag(target, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(target, bambuStatus.camera_recording_when_printing || bambuStatus.camera_timelapse ? lv_color_hex(0x2aff00) : lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(target, bambuStatus.camera_timelapse ? "y 2" : "y");
    }
    else
    {
        lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    }
}

int printingLevelToIndex(int lvl)
{

    switch (lvl)
    {
    case XTOUCH_SPEED_LEVEL_INVALID:
        return 1;
    case XTOUCH_SPEED_LEVEL_SILENCE:
        return 0;
    case XTOUCH_SPEED_LEVEL_NORMAL:
        return 1;
    case XTOUCH_SPEED_LEVEL_RAPID:
        return 2;
    case XTOUCH_SPEED_LEVEL_RAMPAGE:
        return 3;
    }
    return 1; /* Unknown speed levels use Standard instead of undefined data. */
}

void onXTouchPrintStatus(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **comp_homeComponent = lv_event_get_user_data(e);
    static char previewTask[sizeof(bambuStatus.subtask_name)];
    static unsigned previewOwner=~0u;
    unsigned owner=xtouch_printer_active();
    bool taskChanged=owner!=previewOwner||strcmp(previewTask,bambuStatus.subtask_name);
    if(taskChanged){
        ui_home_preview_set(NULL);previewOwner=owner;
        snprintf(previewTask,sizeof(previewTask),"%s",bambuStatus.subtask_name);
    }

    static int confirmationState=-1;
    if(taskChanged||confirmationState!=bambuStatus.print_status)
        ui_confirmPanel_state_changed();
    confirmationState=bambuStatus.print_status;

    char layerText[32];
    sprintf(layerText, "%d/%d", bambuStatus.current_layer, bambuStatus.total_layers);
    lv_label_set_text(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER2_MAINSCREENLAYER], layerText);
    lv_slider_set_value(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENPROGRESSBAR], bambuStatus.mc_print_percent, LV_ANIM_ON);

    char remainingTimeText[48];
    _ui_seconds_to_timeleft(bambuStatus.mc_left_time, remainingTimeText);

    char percentAndRemaining[100]; // Adjust the size accordingly
    snprintf(percentAndRemaining,sizeof(percentAndRemaining), "%d%% %s", bambuStatus.mc_print_percent, remainingTimeText);

    lv_label_set_text(target, percentAndRemaining);
    ui_layout_print_metrics(lv_obj_get_parent(lv_obj_get_parent(target)));
    ui_layout_print_metric(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER2]);

    lv_obj_t *playPauseButton = comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER_MAINSCREENPLAYPAUSEBUTTON];
    lv_obj_t *dropDown = comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL_MAINSCREENSPEEDDROPDOWN];

    switch (isPrinterNoticeActive() ? XTOUCH_PRINT_STATUS_IDLE : bambuStatus.print_status)
    {
    case XTOUCH_PRINT_STATUS_PAUSED:
        lv_label_set_text(playPauseButton, "z");
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENIDLE], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(dropDown, LV_STATE_DISABLED);

        break;
    case XTOUCH_PRINT_STATUS_RUNNING:

        lv_label_set_text(playPauseButton, "0");
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENIDLE], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(dropDown, LV_STATE_DISABLED);
        break;
    case XTOUCH_PRINT_STATUS_PREPARE:
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENIDLE], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(dropDown, LV_STATE_DISABLED);
        break;

    case XTOUCH_PRINT_STATUS_IDLE:
    case XTOUCH_PRINT_STATUS_FINISHED:
    case XTOUCH_PRINT_STATUS_FAILED:
        lv_obj_add_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(comp_homeComponent[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENIDLE], LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *statusCaption = ui_comp_get_child(ui_mainStatusComponent, UI_COMP_MAINSCREENSTATUS_MAINSCREENSTATUSCAPTION);
        if (isPrinterNoticeActive()) {
            lv_obj_set_style_text_color(statusCaption, lv_color_hex(0x59D5E8), 0);
            lv_label_set_text_fmt(statusCaption, XTOUCH_TEXT_SWITCHING_PRINTER, xtouch_printer_name());
        } else {
            lv_obj_set_style_text_color(statusCaption, lv_color_white(), 0);
            lv_label_set_text(statusCaption, xtouch_device_get_print_state());
        }
        lv_obj_add_state(dropDown, LV_STATE_DISABLED);
        break;
    }

    lv_obj_clear_state(playPauseButton, LV_STATE_DISABLED);
    ui_layout_print_button(playPauseButton);

    lv_dropdown_set_selected(dropDown, printingLevelToIndex(bambuStatus.printing_speed_lvl));
}

// COMPONENT homeComponent

lv_obj_t *ui_homeComponent_create(lv_obj_t *comp_parent)
{

    lv_obj_t *cui_homeComponent;
    cui_homeComponent = lv_obj_create(comp_parent);
    lv_obj_set_width(cui_homeComponent, 272);
    lv_obj_set_height(cui_homeComponent, lv_pct(100));
    lv_obj_set_x(cui_homeComponent, 386);
    lv_obj_set_y(cui_homeComponent, 178);
    lv_obj_set_flex_flow(cui_homeComponent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_homeComponent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_homeComponent, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_homeComponent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_homeComponent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_homeComponent, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_homeComponent, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_homeComponent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_homeComponent, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_homeComponent, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_homeComponent, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_homeComponent, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_homeComponent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_homeComponent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_homeComponent, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_homeComponent, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenLeft;
    cui_mainScreenLeft = lv_obj_create(cui_homeComponent);
    lv_obj_set_width(cui_mainScreenLeft, 200);
    lv_obj_set_height(cui_mainScreenLeft, lv_pct(100));
    lv_obj_set_x(cui_mainScreenLeft, 386);
    lv_obj_set_y(cui_mainScreenLeft, 178);
    lv_obj_set_flex_flow(cui_mainScreenLeft, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenLeft, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenLeft, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenLeft, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenLeft, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenLeft, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenLeft, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenLeft, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenLeft, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenLeft, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenLeft, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenLeft, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenLeft, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenLeft, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenLeft, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenLeft, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenStatusBar;
    cui_mainScreenStatusBar = lv_obj_create(cui_mainScreenLeft);
    lv_obj_set_height(cui_mainScreenStatusBar, 28);
    lv_obj_set_width(cui_mainScreenStatusBar, lv_pct(100));
    lv_obj_set_x(cui_mainScreenStatusBar, 386);
    lv_obj_set_y(cui_mainScreenStatusBar, 178);
    lv_obj_set_flex_flow(cui_mainScreenStatusBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenStatusBar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenStatusBar, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenStatusBar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenStatusBar, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenStatusBar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenStatusBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenStatusBar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenStatusBar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenStatusBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenStatusBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenStatusBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenStatusBar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenStatusBar, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenStatusBar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenWifiIcon;
    lv_obj_t *connection = lv_obj_create(cui_mainScreenStatusBar);
    lv_obj_remove_style_all(connection);
    lv_obj_set_size(connection,LV_SIZE_CONTENT,lv_pct(100));
    lv_obj_set_flex_flow(connection,LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(connection,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(connection,2,0);
    lv_obj_clear_flag(connection,LV_OBJ_FLAG_SCROLLABLE);
#if defined(XTOUCH_HAS_CAMERA) && XTOUCH_HAS_CAMERA
    if(xtouch_camera_available())lv_obj_add_flag(connection,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(connection,cameraClick,LV_EVENT_CLICKED,NULL);
#endif
    cui_mainScreenWifiIcon = lv_label_create(connection);
    lv_obj_set_width(cui_mainScreenWifiIcon, LV_SIZE_CONTENT);  /// 50
    lv_obj_set_height(cui_mainScreenWifiIcon, LV_SIZE_CONTENT); /// 16
    lv_label_set_text(cui_mainScreenWifiIcon, "x");
    lv_obj_clear_flag(cui_mainScreenWifiIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenWifiIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenWifiIcon, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *signal=lv_label_create(connection);
    lv_obj_set_style_text_font(signal,&ui_font_text_14,0);
    char rssi[16];snprintf(rssi,sizeof(rssi),bambuStatus.wifi_signal_valid?"%d":"--",-bambuStatus.wifi_signal);
    lv_label_set_text(signal,rssi);
    lv_obj_t *badge=ui_layout_status_create(connection,cui_mainScreenWifiIcon,signal);
    lv_obj_t *statusSpacer=lv_obj_create(cui_mainScreenStatusBar);lv_obj_remove_style_all(statusSpacer);lv_obj_set_size(statusSpacer,0,lv_pct(100));lv_obj_set_flex_grow(statusSpacer,1);lv_obj_clear_flag(statusSpacer,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(badge,0);lv_obj_add_flag(badge,LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_add_event_cb(signal,printerSignal,LV_EVENT_MSG_RECEIVED,NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_WIFI_SIGNAL,signal,NULL);
    lv_obj_t *cui_mainScreenCameraIcon;
    cui_mainScreenCameraIcon = lv_label_create(connection);
    lv_obj_set_height(cui_mainScreenCameraIcon, LV_SIZE_CONTENT); /// 16
    lv_obj_set_width(cui_mainScreenCameraIcon,LV_SIZE_CONTENT);
    lv_label_set_text(cui_mainScreenCameraIcon, "y");
#if defined(XTOUCH_HAS_CAMERA) && XTOUCH_HAS_CAMERA
    if(!xtouch_camera_available())lv_obj_add_flag(cui_mainScreenCameraIcon,LV_OBJ_FLAG_HIDDEN);
#endif
    lv_obj_clear_flag(cui_mainScreenCameraIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenCameraIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenCameraIcon, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenCameraIcon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenCameraIcon, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenCameraIcon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenCameraIcon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenAMS;
    cui_mainScreenAMS = lv_obj_create(cui_mainScreenStatusBar);
    lv_obj_set_height(cui_mainScreenAMS, lv_pct(70));
    lv_obj_set_flex_grow(cui_mainScreenAMS, 1);
    lv_obj_set_x(cui_mainScreenAMS, 386);
    lv_obj_set_y(cui_mainScreenAMS, 178);
    lv_obj_set_flex_flow(cui_mainScreenAMS, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenAMS, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenAMS, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenAMS, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenAMS, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenAMS, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenAMS, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenAMS, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenAMS, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* Four real AMS slots; device selection has its own card. */
    lv_obj_set_width(cui_mainScreenAMS, 0);
    lv_obj_set_flex_grow(cui_mainScreenWifiIcon, 0);
    lv_obj_set_flex_grow(cui_mainScreenCameraIcon, 0);

    lv_obj_t *cui_mainScreenAMSColor0;
    cui_mainScreenAMSColor0 = lv_obj_create(cui_mainScreenAMS);
    lv_obj_set_height(cui_mainScreenAMSColor0, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenAMSColor0, 1);
    lv_obj_set_x(cui_mainScreenAMSColor0, 386);
    lv_obj_set_y(cui_mainScreenAMSColor0, 178);
    lv_obj_set_flex_flow(cui_mainScreenAMSColor0, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenAMSColor0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenAMSColor0, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenAMSColor0, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenAMSColor0, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenAMSColor0, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenAMSColor0, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenAMSColor0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenAMSColor0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenAMSColor0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenAMSColor0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenAMSColor0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenAMSColor0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenAMSColor0, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenAMSColor0, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenAMSColor0, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenAMSColor1;
    cui_mainScreenAMSColor1 = lv_obj_create(cui_mainScreenAMS);
    lv_obj_set_height(cui_mainScreenAMSColor1, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenAMSColor1, 1);
    lv_obj_set_x(cui_mainScreenAMSColor1, 386);
    lv_obj_set_y(cui_mainScreenAMSColor1, 178);
    lv_obj_set_flex_flow(cui_mainScreenAMSColor1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenAMSColor1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenAMSColor1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenAMSColor1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenAMSColor1, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenAMSColor1, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenAMSColor1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenAMSColor1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenAMSColor1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenAMSColor1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenAMSColor1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenAMSColor1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenAMSColor1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenAMSColor1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenAMSColor1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenAMSColor1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenAMSColor2;
    cui_mainScreenAMSColor2 = lv_obj_create(cui_mainScreenAMS);
    lv_obj_set_height(cui_mainScreenAMSColor2, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenAMSColor2, 1);
    lv_obj_set_x(cui_mainScreenAMSColor2, 386);
    lv_obj_set_y(cui_mainScreenAMSColor2, 178);
    lv_obj_set_flex_flow(cui_mainScreenAMSColor2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenAMSColor2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenAMSColor2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenAMSColor2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenAMSColor2, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenAMSColor2, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenAMSColor2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenAMSColor2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenAMSColor2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenAMSColor2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenAMSColor2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenAMSColor2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenAMSColor2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenAMSColor2, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenAMSColor2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenAMSColor2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenAMSColor3;
    cui_mainScreenAMSColor3 = lv_obj_create(cui_mainScreenAMS);
    lv_obj_set_height(cui_mainScreenAMSColor3, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenAMSColor3, 1);
    lv_obj_set_x(cui_mainScreenAMSColor3, 386);
    lv_obj_set_y(cui_mainScreenAMSColor3, 178);
    lv_obj_set_flex_flow(cui_mainScreenAMSColor3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenAMSColor3, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenAMSColor3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenAMSColor3, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenAMSColor3, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenAMSColor3, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenAMSColor3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenAMSColor3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenAMSColor3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenAMSColor3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenAMSColor3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenAMSColor3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenAMSColor3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenAMSColor3, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenAMSColor3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenAMSColor3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_mainStatusComponent = ui_mainScreenStatus_create(cui_mainScreenLeft);

    lv_obj_t *cui_mainScreenPlayer;
    cui_mainScreenPlayer = lv_obj_create(cui_mainScreenLeft);
    lv_obj_set_width(cui_mainScreenPlayer, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenPlayer, 3);
    lv_obj_set_x(cui_mainScreenPlayer, 386);
    lv_obj_set_y(cui_mainScreenPlayer, 178);
    lv_obj_set_flex_flow(cui_mainScreenPlayer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenPlayer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenPlayer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenPlayer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenPlayer, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenPlayer, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenPlayer, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenPlayer, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenPlayer, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenPlayer, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenPlayer, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenPlayer, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenPlayer, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenPlayer, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenPlayer, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenController;
    cui_mainScreenController = lv_obj_create(cui_mainScreenPlayer);
    lv_obj_set_width(cui_mainScreenController, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenController, 4);
    lv_obj_set_x(cui_mainScreenController, 364);
    lv_obj_set_y(cui_mainScreenController, 183);
    lv_obj_set_flex_flow(cui_mainScreenController, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenController, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenController, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenController, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenController, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenController, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenController, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenController, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenNozzleIcon;
    cui_mainScreenNozzleIcon = lv_label_create(cui_mainScreenController);
    lv_obj_set_width(cui_mainScreenNozzleIcon, LV_SIZE_CONTENT);  /// 50
    lv_obj_set_height(cui_mainScreenNozzleIcon, LV_SIZE_CONTENT); /// 24
    lv_label_set_text(cui_mainScreenNozzleIcon, "p");
    lv_obj_clear_flag(cui_mainScreenNozzleIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenNozzleIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenNozzleIcon, &ui_font_xlcd, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenPlayPauseButton;
    cui_mainScreenPlayPauseButton = lv_label_create(cui_mainScreenController);
    lv_obj_set_height(cui_mainScreenPlayPauseButton, LV_SIZE_CONTENT); /// 48
    lv_obj_set_flex_grow(cui_mainScreenPlayPauseButton, 1);
    lv_obj_set_align(cui_mainScreenPlayPauseButton, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenPlayPauseButton, "0");
    lv_obj_add_flag(cui_mainScreenPlayPauseButton, LV_OBJ_FLAG_CLICKABLE);                                                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_mainScreenPlayPauseButton, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenPlayPauseButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_align(cui_mainScreenPlayPauseButton, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cui_mainScreenPlayPauseButton, &ui_font_xlcd, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_mainScreenPlayPauseButton, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cui_mainScreenPlayPauseButton, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(cui_mainScreenPlayPauseButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenPlayPauseButton, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenPlayPauseButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenPlayPauseButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenPlayPauseButton, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenPlayPauseButton, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenPlayPauseButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenPlayPauseButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenPlayPauseButton, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_mainScreenPlayPauseButton, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *cui_mainScreenStopButton;
    cui_mainScreenStopButton = lv_label_create(cui_mainScreenController);
    lv_obj_set_height(cui_mainScreenStopButton, LV_SIZE_CONTENT); /// 48
    lv_obj_set_flex_grow(cui_mainScreenStopButton, 1);
    lv_obj_set_align(cui_mainScreenStopButton, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenStopButton, "1");
    lv_obj_add_flag(cui_mainScreenStopButton, LV_OBJ_FLAG_CLICKABLE);                                                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_mainScreenStopButton, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenStopButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_align(cui_mainScreenStopButton, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cui_mainScreenStopButton, &ui_font_xlcd, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_mainScreenStopButton, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cui_mainScreenStopButton, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(cui_mainScreenStopButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenStopButton, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenStopButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenStopButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenStopButton, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenStopButton, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenStopButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenStopButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenStopButton, lv_color_hex(0xff682a), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_mainScreenStopButton, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *cui_mainScreenProgressBar;
    cui_mainScreenProgressBar = lv_slider_create(cui_mainScreenPlayer);
    lv_slider_set_value(cui_mainScreenProgressBar, 60, LV_ANIM_OFF);
    if (lv_slider_get_mode(cui_mainScreenProgressBar) == LV_SLIDER_MODE_RANGE)
        lv_slider_set_left_value(cui_mainScreenProgressBar, 0, LV_ANIM_OFF);
    lv_obj_set_width(cui_mainScreenProgressBar, 146);
    lv_obj_set_height(cui_mainScreenProgressBar, 14);
    lv_obj_set_align(cui_mainScreenProgressBar, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(cui_mainScreenProgressBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenProgressBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenProgressBar, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE); /// Flags
    lv_obj_set_style_bg_color(cui_mainScreenProgressBar, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenProgressBar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(cui_mainScreenProgressBar, lv_color_hex(0x00FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenProgressBar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(cui_mainScreenProgressBar, lv_color_hex(0xCCCCCC), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenProgressBar, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenProgressBar, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenProgressBar, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenProgressBar, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenProgressBar, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenController1;
    cui_mainScreenController1 = lv_obj_create(cui_mainScreenPlayer);
    lv_obj_set_width(cui_mainScreenController1, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenController1, 1);
    lv_obj_set_x(cui_mainScreenController1, 364);
    lv_obj_set_y(cui_mainScreenController1, 183);
    lv_obj_set_flex_flow(cui_mainScreenController1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenController1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenController1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenController1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenController1, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenController1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenController1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenController1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenController1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenTimeLeftIcon;
    cui_mainScreenTimeLeftIcon = lv_label_create(cui_mainScreenController1);
    lv_obj_set_width(cui_mainScreenTimeLeftIcon, LV_SIZE_CONTENT);  /// 50
    lv_obj_set_height(cui_mainScreenTimeLeftIcon, LV_SIZE_CONTENT); /// 16
    lv_label_set_text(cui_mainScreenTimeLeftIcon, "2");
    lv_obj_clear_flag(cui_mainScreenTimeLeftIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenTimeLeftIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenTimeLeftIcon, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenTimeLeft;
    cui_mainScreenTimeLeft = lv_label_create(cui_mainScreenController1);
    lv_obj_set_width(cui_mainScreenTimeLeft, LV_SIZE_CONTENT);  /// 3
    lv_obj_set_height(cui_mainScreenTimeLeft, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenTimeLeft, LV_ALIGN_CENTER);
    lv_label_set_long_mode(cui_mainScreenTimeLeft, LV_LABEL_LONG_CLIP);
    lv_label_set_text(cui_mainScreenTimeLeft, "N/A");
    lv_obj_clear_flag(cui_mainScreenTimeLeft, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenTimeLeft, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenTimeLeft, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenController2;
    cui_mainScreenController2 = lv_obj_create(cui_mainScreenPlayer);
    lv_obj_set_width(cui_mainScreenController2, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenController2, 1);
    lv_obj_set_x(cui_mainScreenController2, 364);
    lv_obj_set_y(cui_mainScreenController2, 183);
    lv_obj_set_flex_flow(cui_mainScreenController2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenController2, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenController2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenController2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenController2, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenController2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenController2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenController2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenController2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenLayerIcon;
    cui_mainScreenLayerIcon = lv_label_create(cui_mainScreenController2);
    lv_obj_set_width(cui_mainScreenLayerIcon, LV_SIZE_CONTENT);  /// 50
    lv_obj_set_height(cui_mainScreenLayerIcon, LV_SIZE_CONTENT); /// 16
    lv_label_set_text(cui_mainScreenLayerIcon, "3");
    lv_obj_clear_flag(cui_mainScreenLayerIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenLayerIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenLayerIcon, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenLayer;
    cui_mainScreenLayer = lv_label_create(cui_mainScreenController2);
    lv_obj_set_width(cui_mainScreenLayer, LV_SIZE_CONTENT);  /// 3
    lv_obj_set_height(cui_mainScreenLayer, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenLayer, LV_ALIGN_CENTER);
    lv_label_set_long_mode(cui_mainScreenLayer, LV_LABEL_LONG_CLIP);
    lv_label_set_text(cui_mainScreenLayer, "100/255");
    lv_obj_clear_flag(cui_mainScreenLayer, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenLayer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenLayer, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Shared printing layout: vertical controls, fitted preview, thin progress. */
    lv_obj_set_style_pad_all(cui_mainScreenPlayer,6,0);
    lv_obj_set_style_pad_row(cui_mainScreenPlayer,4,0);
    lv_obj_set_flex_align(cui_mainScreenPlayer,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_START);
    lv_obj_t *printTop=lv_obj_create(cui_mainScreenPlayer);
    lv_obj_remove_style_all(printTop);lv_obj_set_width(printTop,lv_pct(100));lv_obj_set_height(printTop,0);lv_obj_set_flex_grow(printTop,1);
    lv_obj_set_flex_flow(printTop,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(printTop,6,0);
    lv_obj_clear_flag(printTop,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(printTop,0);
    lv_obj_set_parent(cui_mainScreenController,printTop);
    lv_obj_set_width(cui_mainScreenController,40);lv_obj_set_height(cui_mainScreenController,lv_pct(100));lv_obj_set_flex_grow(cui_mainScreenController,0);
    lv_obj_set_flex_flow(cui_mainScreenController,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenController,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cui_mainScreenController,4,0);
    lv_obj_add_flag(cui_mainScreenNozzleIcon,LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *printButtons[]={cui_mainScreenPlayPauseButton,cui_mainScreenStopButton};
    for(unsigned i=0;i<2;++i){
        lv_obj_set_width(printButtons[i],lv_pct(100));lv_obj_set_height(printButtons[i],0);lv_obj_set_flex_grow(printButtons[i],1);
        lv_obj_set_style_pad_top(printButtons[i],0,0);lv_obj_set_style_pad_bottom(printButtons[i],0,0);
        lv_label_set_long_mode(printButtons[i],LV_LABEL_LONG_CLIP);
        lv_obj_add_event_cb(printButtons[i],print_button_layout,LV_EVENT_SIZE_CHANGED,NULL);
    }
    printPreviewBox=lv_obj_create(printTop);lv_obj_remove_style_all(printPreviewBox);
    lv_obj_set_width(printPreviewBox,0);lv_obj_set_height(printPreviewBox,lv_pct(100));lv_obj_set_flex_grow(printPreviewBox,1);
    lv_obj_clear_flag(printPreviewBox,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    printPreviewEmpty=lv_label_create(printPreviewBox);lv_label_set_text(printPreviewEmpty,LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(printPreviewEmpty,&lv_font_montserrat_14,0);lv_obj_set_style_text_color(printPreviewEmpty,lv_color_hex(0xAAAAAA),0);lv_obj_center(printPreviewEmpty);
    printPreviewImage=lv_img_create(printPreviewBox);lv_obj_add_flag(printPreviewImage,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(printPreviewBox,print_preview_fit,LV_EVENT_SIZE_CHANGED,NULL);
    lv_obj_add_event_cb(printPreviewBox,print_preview_deleted,LV_EVENT_DELETE,NULL);
    lv_obj_set_width(cui_mainScreenProgressBar,lv_pct(100));lv_obj_set_height(cui_mainScreenProgressBar,4);
    lv_obj_set_height(cui_mainScreenController1,19);lv_obj_set_flex_grow(cui_mainScreenController1,0);
    lv_obj_set_height(cui_mainScreenController2,19);lv_obj_set_flex_grow(cui_mainScreenController2,0);
    lv_obj_t *printMetrics=lv_obj_create(cui_mainScreenPlayer);lv_obj_remove_style_all(printMetrics);
    lv_obj_set_width(printMetrics,lv_pct(100));lv_obj_set_height(printMetrics,21);
    lv_obj_set_layout(printMetrics,0);
    lv_obj_clear_flag(printMetrics,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_parent(cui_mainScreenController1,printMetrics);lv_obj_set_parent(cui_mainScreenController2,printMetrics);
    lv_obj_set_width(cui_mainScreenController1,0);lv_obj_set_flex_grow(cui_mainScreenController1,2);lv_obj_set_height(cui_mainScreenController1,lv_pct(100));
    lv_obj_set_width(cui_mainScreenController2,0);lv_obj_set_flex_grow(cui_mainScreenController2,1);lv_obj_set_height(cui_mainScreenController2,lv_pct(100));
    lv_obj_set_layout(cui_mainScreenController1,0);lv_obj_set_layout(cui_mainScreenController2,0);
    lv_obj_add_event_cb(cui_mainScreenController1,ui_layout_print_metric_event,LV_EVENT_SIZE_CHANGED,NULL);
    lv_obj_add_event_cb(cui_mainScreenController2,ui_layout_print_metric_event,LV_EVENT_SIZE_CHANGED,NULL);
    lv_obj_add_event_cb(printMetrics,ui_layout_print_metrics_event,LV_EVENT_SIZE_CHANGED,NULL);

    lv_obj_t *cui_mainScreenCentral;
    cui_mainScreenCentral = lv_obj_create(cui_mainScreenLeft);
    lv_obj_set_width(cui_mainScreenCentral, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenCentral, 1);
    lv_obj_set_x(cui_mainScreenCentral, 386);
    lv_obj_set_y(cui_mainScreenCentral, 178);
    lv_obj_set_flex_flow(cui_mainScreenCentral, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenCentral, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenCentral, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenCentral, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenCentral, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenCentral, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenCentral, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenCentral, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenCentral, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenCentral, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenCentral, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenCentral, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenCentral, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenCentral, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenCentral, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenSpeedIcon;
    cui_mainScreenSpeedIcon = lv_label_create(cui_mainScreenCentral);
    lv_obj_set_width(cui_mainScreenSpeedIcon, LV_SIZE_CONTENT);  /// 50
    lv_obj_set_height(cui_mainScreenSpeedIcon, LV_SIZE_CONTENT); /// 16
    lv_label_set_text(cui_mainScreenSpeedIcon, "h");
    lv_obj_clear_flag(cui_mainScreenSpeedIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenSpeedIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenSpeedIcon, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenSpeedDropDown;
    cui_mainScreenSpeedDropDown = lv_dropdown_create(cui_mainScreenCentral);
    lv_dropdown_set_options(cui_mainScreenSpeedDropDown, XTOUCH_TEXT_SPEED_OPTIONS);
    lv_obj_set_width(cui_mainScreenSpeedDropDown, 150);
    lv_obj_set_height(cui_mainScreenSpeedDropDown, LV_SIZE_CONTENT);           /// 1
    lv_obj_add_flag(cui_mainScreenSpeedDropDown, LV_OBJ_FLAG_SCROLL_ON_FOCUS); /// Flags
    lv_obj_set_style_bg_color(cui_mainScreenSpeedDropDown, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenSpeedDropDown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenSpeedDropDown, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_dropdown_set_selected(cui_mainScreenSpeedDropDown, 1);

    lv_obj_set_style_text_color(cui_mainScreenSpeedDropDown, lv_color_hex(0x2aff00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenSpeedDropDown, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_text_letter_space(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_letter_space(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 32, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), lv_color_hex(0x000000), LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 255, LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), lv_color_hex(0x2aff00), LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 255, LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), lv_color_hex(0x000000), LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 255, LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), lv_color_hex(0x00AA00), LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(lv_dropdown_get_list(cui_mainScreenSpeedDropDown), 255, LV_PART_SELECTED | LV_STATE_PRESSED);

    lv_obj_t *cui_mainScreenRight;
    cui_mainScreenRight = lv_obj_create(cui_homeComponent);
    lv_obj_set_width(cui_mainScreenRight, 72);
    lv_obj_set_height(cui_mainScreenRight, lv_pct(100));
    lv_obj_set_x(cui_mainScreenRight, 386);
    lv_obj_set_y(cui_mainScreenRight, 178);
    lv_obj_set_flex_flow(cui_mainScreenRight, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenRight, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenRight, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenRight, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_mainScreenRight, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenRight, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenRight, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenRight, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenRight, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenRight, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenRight, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenRight, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenRight, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenRight, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenRight, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenRight, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Square device selector with separate model and scrolling profile-name rows.
    lv_obj_t *deviceButton = lv_obj_create(cui_mainScreenRight);
    lv_obj_set_size(deviceButton, lv_pct(100), 49);
    lv_obj_set_flex_flow(deviceButton, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(deviceButton, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(deviceButton, 0, 0);
    lv_obj_set_style_pad_row(deviceButton, 0, 0);
    lv_obj_set_style_border_width(deviceButton, 0, 0);
    lv_obj_set_style_radius(deviceButton, 6, 0);
    lv_obj_set_style_bg_color(deviceButton, lv_color_hex(0x555555), 0);
    lv_obj_clear_flag(deviceButton, LV_OBJ_FLAG_SCROLLABLE);
    // SHORT_CLICKED excludes release after a long press: opening the picker
    // must never also cycle to the next printer.
    lv_obj_add_event_cb(deviceButton, on_next_printer, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(deviceButton, on_next_printer, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_t *modelRow=lv_obj_create(deviceButton);
    lv_obj_remove_style_all(modelRow);
    lv_obj_set_size(modelRow,64,22);
    lv_obj_clear_flag(modelRow,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(modelRow,ui_layout_device_model_event,LV_EVENT_SIZE_CHANGED,NULL);
    lv_obj_t *deviceIcon = lv_label_create(modelRow);
    lv_label_set_text(deviceIcon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(deviceIcon, lv_color_hex(0x59D5E8), 0);
    lv_obj_t *modelName=lv_label_create(modelRow);
    lv_label_set_long_mode(modelName,LV_LABEL_LONG_CLIP);
    set_device_model(modelName);
    lv_obj_add_event_cb(modelName,device_model_updated,LV_EVENT_MSG_RECEIVED,NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_MODEL,modelName,NULL);
    lv_obj_t *deviceName = lv_label_create(deviceButton);
    lv_obj_set_size(deviceName,64,20);
    lv_obj_set_style_text_align(deviceName,LV_TEXT_ALIGN_CENTER,0);
    lv_label_set_long_mode(deviceName, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(deviceName, 25, 0);
    lv_label_set_text(deviceName, xtouch_printer_name());
    lv_obj_set_style_text_color(deviceName,lv_color_hex(xtouch_printer_source_color(xtouch_printer_active())),0);

    lv_obj_t *cui_mainScreenLightButton;
    cui_mainScreenLightButton = lv_obj_create(cui_mainScreenRight);
    lv_obj_set_width(cui_mainScreenLightButton, lv_pct(100));
    lv_obj_set_height(cui_mainScreenLightButton, 49);
    lv_obj_set_x(cui_mainScreenLightButton, 386);
    lv_obj_set_y(cui_mainScreenLightButton, 178);
    lv_obj_set_flex_flow(cui_mainScreenLightButton, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_mainScreenLightButton, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // lv_obj_add_state(cui_mainScreenLightButton, LV_STATE_CHECKED);                                                                                                                                                                                                          /// States
    lv_obj_clear_flag(cui_mainScreenLightButton, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenLightButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenLightButton, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenLightButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenLightButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenLightButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenLightButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenLightButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenLightButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenLightButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenLightButton, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenLightButton, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenLightButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cui_mainScreenLightButton, lv_color_hex(0x2aff00), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_opa(cui_mainScreenLightButton, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(cui_mainScreenLightButton, 3, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(cui_mainScreenLightButton, lv_color_hex(0x2aff00), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(cui_mainScreenLightButton, 255, LV_PART_MAIN | LV_STATE_CHECKED);

    lv_obj_t *cui_mainScreenLightButtonIcon;
    cui_mainScreenLightButtonIcon = lv_label_create(cui_mainScreenLightButton);
    lv_obj_set_width(cui_mainScreenLightButtonIcon, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenLightButtonIcon, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenLightButtonIcon, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenLightButtonIcon, "w");
    lv_obj_clear_flag(cui_mainScreenLightButtonIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenLightButtonIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_mainScreenLightButtonIcon, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_layout_lamp(cui_mainScreenLightButton,cui_mainScreenLightButtonIcon);

    lv_obj_t *cui_mainScreenTemps;
    cui_mainScreenTemps = lv_obj_create(cui_mainScreenRight);
    lv_obj_set_height(cui_mainScreenTemps, 130);
    lv_obj_set_width(cui_mainScreenTemps, lv_pct(100));
    lv_obj_set_x(cui_mainScreenTemps, 386);
    lv_obj_set_y(cui_mainScreenTemps, 178);
    lv_obj_set_flex_flow(cui_mainScreenTemps, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenTemps, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cui_mainScreenTemps, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenTemps, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenTemps, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenTemps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenTemps, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenTemps, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenTemps, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenTemps, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenTemps, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenTemps, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenTemps, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenTemps, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenTemps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenNozzleTemp;
    cui_mainScreenNozzleTemp = lv_obj_create(cui_mainScreenTemps);
    lv_obj_set_width(cui_mainScreenNozzleTemp, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenNozzleTemp, 2);
    lv_obj_set_x(cui_mainScreenNozzleTemp, 386);
    lv_obj_set_y(cui_mainScreenNozzleTemp, 178);
    lv_obj_set_flex_flow(cui_mainScreenNozzleTemp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenNozzleTemp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenNozzleTemp, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenNozzleTemp, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenNozzleTemp, lv_color_hex(0x2aff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenNozzleTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenNozzleTemp, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenNozzleTemp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenNozzleTemp, lv_color_hex(0x2aff00), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_mainScreenNozzleTemp, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *cui_mainScreenNozzleTempIcon;
    cui_mainScreenNozzleTempIcon = lv_label_create(cui_mainScreenNozzleTemp);
    lv_obj_set_width(cui_mainScreenNozzleTempIcon, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenNozzleTempIcon, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenNozzleTempIcon, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenNozzleTempIcon, "f");
    lv_obj_set_style_text_color(cui_mainScreenNozzleTempIcon, lv_color_hex(0xCCCCCC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenNozzleTempIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cui_mainScreenNozzleTempIcon, &ui_font_xlcd, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenNozzleTempValue;
    cui_mainScreenNozzleTempValue = lv_label_create(cui_mainScreenNozzleTemp);
    lv_obj_set_width(cui_mainScreenNozzleTempValue, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenNozzleTempValue, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenNozzleTempValue, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenNozzleTempValue, "--");
    lv_obj_set_style_text_font(cui_mainScreenNozzleTempValue, XTOUCH_FONT_NUMERIC, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenBedTemp;
    cui_mainScreenBedTemp = lv_obj_create(cui_mainScreenTemps);
    lv_obj_set_width(cui_mainScreenBedTemp, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenBedTemp, 2);
    lv_obj_set_x(cui_mainScreenBedTemp, 386);
    lv_obj_set_y(cui_mainScreenBedTemp, 178);
    lv_obj_set_flex_flow(cui_mainScreenBedTemp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenBedTemp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenBedTemp, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenBedTemp, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenBedTemp, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenBedTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenBedTemp, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenBedTemp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_mainScreenBedTemp, lv_color_hex(0x2aff00), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_mainScreenBedTemp, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *cui_mainScreenBedTempIcon;
    cui_mainScreenBedTempIcon = lv_label_create(cui_mainScreenBedTemp);
    lv_obj_set_width(cui_mainScreenBedTempIcon, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenBedTempIcon, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenBedTempIcon, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenBedTempIcon, "e");
    lv_obj_set_style_text_color(cui_mainScreenBedTempIcon, lv_color_hex(0xCCCCCC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenBedTempIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cui_mainScreenBedTempIcon, &ui_font_xlcd, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenBedTempValue;
    cui_mainScreenBedTempValue = lv_label_create(cui_mainScreenBedTemp);
    lv_obj_set_width(cui_mainScreenBedTempValue, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenBedTempValue, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenBedTempValue, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenBedTempValue, "--");
    lv_obj_set_style_text_font(cui_mainScreenBedTempValue, XTOUCH_FONT_NUMERIC, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenChamberTemp;
    cui_mainScreenChamberTemp = lv_obj_create(cui_mainScreenTemps);
    lv_obj_set_width(cui_mainScreenChamberTemp, lv_pct(100));
    lv_obj_set_flex_grow(cui_mainScreenChamberTemp, 2);
    lv_obj_set_x(cui_mainScreenChamberTemp, 386);
    lv_obj_set_y(cui_mainScreenChamberTemp, 178);
    lv_obj_set_flex_flow(cui_mainScreenChamberTemp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cui_mainScreenChamberTemp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_mainScreenChamberTemp, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_mainScreenChamberTemp, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_mainScreenChamberTemp, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cui_mainScreenChamberTemp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cui_mainScreenChamberTemp, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenChamberTemp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (!xtouch_bblp_chamber_display())
    {
        lv_obj_add_flag(cui_mainScreenChamberTemp, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t *cui_mainScreenChamberTempIcon;
    cui_mainScreenChamberTempIcon = lv_label_create(cui_mainScreenChamberTemp);
    lv_obj_set_width(cui_mainScreenChamberTempIcon, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenChamberTempIcon, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenChamberTempIcon, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenChamberTempIcon, "g");
    lv_obj_set_style_text_color(cui_mainScreenChamberTempIcon, lv_color_hex(0xCCCCCC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cui_mainScreenChamberTempIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cui_mainScreenChamberTempIcon, &ui_font_xlcd, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_mainScreenChamberTempValue;
    cui_mainScreenChamberTempValue = lv_label_create(cui_mainScreenChamberTemp);
    lv_obj_set_width(cui_mainScreenChamberTempValue, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_mainScreenChamberTempValue, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(cui_mainScreenChamberTempValue, LV_ALIGN_CENTER);
    lv_label_set_text(cui_mainScreenChamberTempValue, "--");
    lv_obj_set_style_text_font(cui_mainScreenChamberTempValue, XTOUCH_FONT_NUMERIC, LV_PART_MAIN | LV_STATE_DEFAULT);

    // The device and lamp cards leave 130px for temperatures. Stacked icon/value
    // pairs would clip, so use compact horizontal rows when chamber is visible.
    if (!lv_obj_has_flag(cui_mainScreenChamberTemp, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_set_style_pad_all(cui_mainScreenTemps, 0, 0);
        lv_obj_t *rows[] = {cui_mainScreenNozzleTemp, cui_mainScreenBedTemp, cui_mainScreenChamberTemp};
        lv_obj_t *icons[] = {cui_mainScreenNozzleTempIcon, cui_mainScreenBedTempIcon, cui_mainScreenChamberTempIcon};
        for (unsigned i=0; i<3; ++i) {
            lv_obj_set_flex_flow(rows[i], LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(rows[i], LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(rows[i], 0, 0);
            lv_obj_set_width(icons[i], 24);
            lv_obj_set_style_text_align(icons[i], LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_set_style_text_font(icons[i], &ui_font_xlcd, 0);
            /* Equal icon columns keep all three readings aligned. */
            lv_obj_t *value = lv_obj_get_child(rows[i], 1);
            lv_obj_set_width(value, 46);
            lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_font(value, &ui_font_text_24, 0);
            lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);
        }
    }

    bool horizontal=!lv_obj_has_flag(cui_mainScreenChamberTemp,LV_OBJ_FLAG_HIDDEN);
    ui_layout_temperature_setup(cui_mainScreenNozzleTemp,horizontal);
    ui_layout_temperature_setup(cui_mainScreenBedTemp,horizontal);
    ui_layout_temperature_setup(cui_mainScreenChamberTemp,true);

    // Keep the compact, square AMS indicators independent of name length.
    lv_obj_set_width(cui_mainScreenStatusBar, lv_pct(100));
    lv_obj_set_style_pad_column(cui_mainScreenStatusBar, 2, 0);
    lv_obj_set_flex_grow(cui_mainScreenCameraIcon, 0);
    lv_obj_set_width(cui_mainScreenCameraIcon, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(cui_mainScreenAMS, 0);
    lv_obj_set_size(cui_mainScreenAMS, 84, 16);
    lv_obj_set_style_pad_column(cui_mainScreenAMS, 2, 0);
    lv_obj_set_flex_align(cui_mainScreenStatusBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(cui_mainScreenStatusBar, 3, 0);
    lv_obj_set_style_pad_right(cui_mainScreenStatusBar, 3, 0);
    lv_obj_t *slots[4] = {cui_mainScreenAMSColor0, cui_mainScreenAMSColor1,
                         cui_mainScreenAMSColor2, cui_mainScreenAMSColor3};
    for (uint8_t i = 0; i < 4; ++i) {
        lv_obj_set_flex_grow(slots[i], 0);
        lv_obj_set_size(slots[i], 16, 16);
        lv_obj_set_style_radius(slots[i], 3, 0);
        lv_obj_clear_flag(slots[i], LV_OBJ_FLAG_CLICKABLE);
    }
    ui_ams_strip_bind(cui_mainScreenAMS);
    ui_home_clock_bind(statusSpacer);

    lv_obj_t **children = lv_mem_alloc(sizeof(lv_obj_t *) * _UI_COMP_HOMECOMPONENT_NUM);
    children[UI_COMP_HOMECOMPONENT_HOMECOMPONENT] = cui_homeComponent;
    children[UI_COMP_HOMECOMPONENT_DEVICEBUTTON] = deviceButton;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT] = cui_mainScreenLeft;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR] = cui_mainScreenStatusBar;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENWIFIICON] = cui_mainScreenWifiIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENCAMERAICON] = cui_mainScreenCameraIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENAMS] = cui_mainScreenAMS;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENAMS_MAINSCREENAMSCOLOR0] = cui_mainScreenAMSColor0;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENAMS_MAINSCREENAMSCOLOR1] = cui_mainScreenAMSColor1;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENAMS_MAINSCREENAMSCOLOR2] = cui_mainScreenAMSColor2;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENSTATUSBAR_MAINSCREENAMS_MAINSCREENAMSCOLOR3] = cui_mainScreenAMSColor3;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENIDLE] = ui_mainStatusComponent;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER] = cui_mainScreenPlayer;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER] = cui_mainScreenController;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER_MAINSCREENNOZZLEICON] = cui_mainScreenNozzleIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER_MAINSCREENPLAYPAUSEBUTTON] = cui_mainScreenPlayPauseButton;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER_MAINSCREENSTOPBUTTON] = cui_mainScreenStopButton;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENPROGRESSBAR] = cui_mainScreenProgressBar;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER1] = cui_mainScreenController1;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER1_MAINSCREENTIMELEFTICON] = cui_mainScreenTimeLeftIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER1_MAINSCREENTIMELEFT] = cui_mainScreenTimeLeft;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER2] = cui_mainScreenController2;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER2_MAINSCREENLAYERICON] = cui_mainScreenLayerIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENPLAYER_MAINSCREENCONTROLLER2_MAINSCREENLAYER] = cui_mainScreenLayer;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL] = cui_mainScreenCentral;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL_MAINSCREENSPEEDICON] = cui_mainScreenSpeedIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENLEFT_MAINSCREENCENTRAL_MAINSCREENSPEEDDROPDOWN] = cui_mainScreenSpeedDropDown;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT] = cui_mainScreenRight;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENLIGHTBUTTON] = cui_mainScreenLightButton;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENLIGHTBUTTON_MAINSCREENLIGHTBUTTONICON] = cui_mainScreenLightButtonIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS] = cui_mainScreenTemps;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENNOZZLETEMP] = cui_mainScreenNozzleTemp;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENNOZZLETEMP_MAINSCREENNOZZLETEMPICON] = cui_mainScreenNozzleTempIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENNOZZLETEMP_MAINSCREENNOZZLETEMPVALUE] = cui_mainScreenNozzleTempValue;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENBEDTEMP] = cui_mainScreenBedTemp;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENBEDTEMP_MAINSCREENBEDTEMPICON] = cui_mainScreenBedTempIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENBEDTEMP_MAINSCREENBEDTEMPVALUE] = cui_mainScreenBedTempValue;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENCHAMBERTEMP] = cui_mainScreenChamberTemp;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENCHAMBERTEMP_MAINSCREENCHAMBERTEMPICON] = cui_mainScreenChamberTempIcon;
    children[UI_COMP_HOMECOMPONENT_MAINSCREENRIGHT_MAINSCREENTEMPS_MAINSCREENCHAMBERTEMP_MAINSCREENCHAMBERTEMPVALUE] = cui_mainScreenChamberTempValue;
    lv_obj_add_event_cb(cui_homeComponent, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children);
    lv_obj_add_event_cb(cui_homeComponent, del_component_child_event_cb, LV_EVENT_DELETE, children);
    lv_obj_add_event_cb(cui_mainScreenPlayPauseButton, ui_event_comp_homeComponent_mainScreenPlayPauseButton, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_mainScreenStopButton, ui_event_comp_homeComponent_mainScreenStopButton, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_mainScreenSpeedDropDown, ui_event_comp_homeComponent_mainScreenSpeedDropDown, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_mainScreenLightButton, ui_event_comp_homeComponent_mainScreenLightButton, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_mainScreenBedTemp, ui_event_comp_homeComponent_mainScreenBedTemp, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_mainScreenNozzleTemp, ui_event_comp_homeComponent_mainScreenNozzleTemp, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_mainScreenSpeedDropDown, ui_event_comp_homeComponent_mainScreenSpeedChange, LV_EVENT_ALL, children);

    lv_obj_add_event_cb(cui_mainScreenLightButton, onXTouchLightMessage, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_LIGHT_REPORT, cui_mainScreenLightButton, NULL);

    lv_obj_add_event_cb(cui_mainScreenBedTempValue, onXTouchBedTemp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_BED_TEMP, cui_mainScreenBedTempValue, NULL);

    lv_obj_add_event_cb(cui_mainScreenBedTempIcon, onXTouchBedTempTarget, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_BED_TARGET_TEMP, cui_mainScreenBedTempIcon, NULL);

    lv_obj_add_event_cb(cui_mainScreenNozzleTempValue, onXTouchBedTemp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_NOZZLE_TEMP, cui_mainScreenNozzleTempValue, NULL);

    lv_obj_add_event_cb(cui_mainScreenChamberTempValue, onXTouchChamberTemp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_CHAMBER_TEMP, cui_mainScreenChamberTempValue, NULL);

    lv_obj_add_event_cb(cui_mainScreenNozzleTempIcon, onXTouchBedTempTarget, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_NOZZLE_TARGET_TEMP, cui_mainScreenNozzleTempIcon, NULL);


    lv_obj_add_event_cb(cui_mainScreenWifiIcon, onXTouchWifiSignal, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_WIFI_SIGNAL, cui_mainScreenWifiIcon, NULL);

#if defined(XTOUCH_HAS_CAMERA) && XTOUCH_HAS_CAMERA
    lv_obj_add_flag(cui_mainScreenCameraIcon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(cui_mainScreenWifiIcon,LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(signal,LV_OBJ_FLAG_EVENT_BUBBLE);
#endif
    lv_obj_add_event_cb(cui_mainScreenCameraIcon, onXTouchIPCam, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_IPCAM, cui_mainScreenCameraIcon, NULL);

    lv_obj_add_event_cb(cui_mainScreenTimeLeft, onXTouchPrintStatus, LV_EVENT_MSG_RECEIVED, children);
    lv_msg_subsribe_obj(XTOUCH_ON_PRINT_STATUS, cui_mainScreenTimeLeft, NULL);

    ui_comp_homeComponent_create_hook(cui_homeComponent);
    return cui_homeComponent;
}
