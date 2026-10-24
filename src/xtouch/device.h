#include "ui/nozzle_details.h"
#include "ui/ams_details.h"
#include "ams_units.h"
#ifndef _XLCD_DEVICE
#define _XLCD_DEVICE

#include <Arduino.h>
#include "cloud.h"
#include "features/power/idle_policy.h"
#include "ui/filament_actions.h"
#include "command_result.h"
static String xtouch_filament_pending_sequence;

#define XTOUCH_DEVICE_CONTROL_MOVE_SPEED_XY 3000
#define XTOUCH_DEVICE_CONTROL_MOVE_SPEED_Z 1500

uint32_t xtouch_device_sequence_id = 0;

String xtouch_device_next_sequence()
{
    xtouch_device_sequence_id++;
    xtouch_device_sequence_id %= (UINT32_MAX - 1);
    return String(xtouch_device_sequence_id);
}

String xtouch_device_print_action(char const *action)
{
    DynamicJsonDocument json(256);
    json["print"]["command"] = action;
    json["print"]["param"] = "";
    json["print"]["sequence_id"] = xtouch_device_next_sequence();

    String result;
    serializeJson(json, result);
    return result;
}

String lastPrintState;
void xtouch_device_set_print_state(String state)
{
    if (state == "IDLE")
        bambuStatus.print_status = XTOUCH_PRINT_STATUS_IDLE;
    else if (state == "RUNNING")
        bambuStatus.print_status = XTOUCH_PRINT_STATUS_RUNNING;
    else if (state == "PAUSE")
        bambuStatus.print_status = XTOUCH_PRINT_STATUS_PAUSED;
    else if (state == "FINISH")
        bambuStatus.print_status = XTOUCH_PRINT_STATUS_FINISHED;
    else if (state == "PREPARE")
        bambuStatus.print_status = XTOUCH_PRINT_STATUS_PREPARE;
    else if (state == "FAILED")
        bambuStatus.print_status = XTOUCH_PRINT_STATUS_FAILED;

    if (xtouch_print_wake_requested(lastPrintState.c_str(),state.c_str(),xTouchConfig.xTouchWakeOnPrint))
    {
        xtouch_screen_wakeUp();
    }

    lastPrintState = state;
}

// Track a short burst (for example two fan commands) rather than losing the
// first rejection as soon as a second request is sent. Never cross profiles.
static struct { uint32_t sequence, sent; unsigned printer; bool used; char command[32]; } xtouch_controls[8];
static unsigned xtouch_control_cursor;
void xtouch_device_control_clear(){for(auto &entry:xtouch_controls)entry.used=false;xtouch_control_cursor=0;}
bool xtouch_device_publish(String request)
{
    if (xtouch_printer_switch_pending() || !xtouch_pubSubClient.connected()) return false;
    bool sent=xtouch_pubSubClient.publish(xtouch_mqtt_request_topic.c_str(), request.c_str());
    StaticJsonDocument<512> summary;
    if(!deserializeJson(summary,request)){
      const char *group=summary.containsKey("print")?"print":summary.containsKey("system")?"system":summary.containsKey("info")?"info":summary.containsKey("pushing")?"pushing":"other";
      JsonObjectConst message=summary[group].as<JsonObjectConst>();
      ConsoleInfo.printf("[MQTT TX] group=%s command=%s sequence=%s sent=%u\n",group,message["command"]|"",message["sequence_id"]|"",(unsigned)sent);
    }
    if(sent&&(request.indexOf("\"print\"")>=0||request.indexOf("\"system\"")>=0)){StaticJsonDocument<512> d;if(!deserializeJson(d,request)){
      JsonObjectConst p=d.containsKey("print")?d["print"].as<JsonObjectConst>():d["system"].as<JsonObjectConst>();
      auto &entry=xtouch_controls[xtouch_control_cursor++%8];
      entry.sequence=p["sequence_id"].as<String>().toInt();entry.sent=millis();entry.printer=xtouch_printer_active();entry.used=true;strlcpy(entry.command,p["command"]|"",sizeof(entry.command));
    }}
    delay(10);
    return sent;
}

bool xtouch_device_scoped_command(const char *name){
 static const char *const commands[]={"ams_get_rfid","ams_change_filament","ams_filament_drying","set_nozzle_temp","select_extruder","set_extrusion_length","holder_nozzle_refresh","nozzle_info_confirm","nozzle_holder_ctrl","set_ctt"};
 for(const char *command:commands)if(!strcmp(name,command))return true;return false;
}
void xtouch_device_control_reply(JsonObjectConst report){
 String sequence=report["sequence_id"].as<String>();
 if(!sequence.length())return;
 char *end=NULL;uint32_t id=strtoul(sequence.c_str(),&end,10);
 if(!end||*end)return;
 for(auto &entry:xtouch_controls)if(entry.used&&entry.sequence==id&&!strcmp(entry.command,report["command"]|"")&&entry.printer==xtouch_printer_active()&&millis()-entry.sent<15000){
  const char *command=report["command"]|"";uint32_t error=report["err_code"]|0UL;
  XtouchReplyResult outcome=xtouch_reply_result(report["result"]|"",error);
  if(outcome==XTOUCH_REPLY_WAIT&&!xtouch_reply_auth_failure(report["reason"]|"",error))return;
  entry.used=false;
  bool authorization=xtouch_reply_auth_failure(report["reason"]|"",error);
  if(sequence==xtouch_filament_pending_sequence)return;
  if((outcome==XTOUCH_REPLY_REJECTED||authorization)&&!xtouch_device_scoped_command(command))ui_filament_report_rejection(error,authorization);
  break;
 }
}

void xtouch_device_get_version()
{
    DynamicJsonDocument json(256);
    json["info"]["command"] = "get_version";
    json["info"]["sequence_id"] = xtouch_device_next_sequence();
    String result;
    serializeJson(json, result);
    xtouch_device_publish(result);
}

void xtouch_device_pushall()
{
    DynamicJsonDocument json(256);
    json["pushing"]["command"] = "pushall";
    json["pushing"]["sequence_id"] = xtouch_device_next_sequence();

    String result;
    serializeJson(json, result);
    xtouch_device_publish(result);
}

void xtouch_device_set_printing_speed(int lvl)
{
    DynamicJsonDocument json(256);
    json["print"]["command"] = "print_speed";
    json["print"]["sequence_id"] = xtouch_device_next_sequence();
    json["print"]["param"] = String(lvl);

    String result;
    serializeJson(json, result);
    xtouch_device_publish(result);
}

void xtouch_device_gcode_line(String line)
{
    DynamicJsonDocument json(line.length() + 256);
    json["print"]["command"] = "gcode_line";
    json["print"]["sequence_id"] = xtouch_device_next_sequence();
    json["print"]["param"] = line.c_str();
    if(xtouch_printer_cloud()&&xtouch_cloud_user()[0]){
        const char *uid=xtouch_cloud_user();
        // MQTT usernames carry u_; the command field is the account ID.
        json["print"]["user_id"]=!strncmp(uid,"u_",2)?uid+2:uid;
    }


    String result;
    serializeJson(json, result);
    xtouch_device_publish(result);
}

void xtouch_device_move_axis(String axis, double value, int speed)
{
    if(!ui_manual_motion_allowed())return;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "M211 S \nM211 X1 Y1 Z1\nM1002 push_ref_mode\nG91 \nG1 %s%0.1f F%d\nM1002 pop_ref_mode\nM211 R\n", axis.c_str(), value, speed);
    xtouch_device_gcode_line(String(cmd));
}

void xtouch_device_onLightToggleCommand(lv_msg_t *m)
{

    DynamicJsonDocument json(256);
    json["system"]["command"] = "ledctrl";
    json["system"]["led_node"] = "chamber_light";
    json["system"]["sequence_id"] = xtouch_device_next_sequence();
    json["system"]["led_mode"] = bambuStatus.chamberLed ? "off" : "on";
    json["system"]["led_on_time"] = 500;
    json["system"]["led_off_time"] = 500;
    json["system"]["loop_times"] = 0;
    json["system"]["interval_time"] = 0;


    String result;
    serializeJson(json, result);
    xtouch_device_publish(result);
    delay(10);
}

void xtouch_device_onHomeCommand(lv_msg_t *m)
{
    if(!ui_manual_motion_allowed())return;
    xtouch_device_gcode_line("G28 \n");
}

void xtouch_device_onLeftCommand(lv_msg_t *m)
{
    xtouch_device_move_axis("X", controlMode.inc == 10 ? -10 : -1, XTOUCH_DEVICE_CONTROL_MOVE_SPEED_XY);
}

void xtouch_device_onRightCommand(lv_msg_t *m)
{
    xtouch_device_move_axis("X", controlMode.inc == 10 ? 10 : 1, XTOUCH_DEVICE_CONTROL_MOVE_SPEED_XY);
}

void xtouch_device_onUpCommand(lv_msg_t *m)
{
    String axis = controlMode.axis == ControlAxisXY ? "Y" : "Z";
    int multiplier = axis == "Y" ? 1 : -1;
    xtouch_device_move_axis(axis, controlMode.inc == 10 ? 10 * multiplier : 1 * multiplier, axis == "Y" ? XTOUCH_DEVICE_CONTROL_MOVE_SPEED_XY : XTOUCH_DEVICE_CONTROL_MOVE_SPEED_Z);
}

void xtouch_device_onDownCommand(lv_msg_t *m)
{
    String axis = controlMode.axis == ControlAxisXY ? "Y" : "Z";
    int multiplier = axis == "Y" ? -1 : 1;
    xtouch_device_move_axis(axis, controlMode.inc == 10 ? 10 * multiplier : 1 * multiplier, axis == "Y" ? XTOUCH_DEVICE_CONTROL_MOVE_SPEED_XY : XTOUCH_DEVICE_CONTROL_MOVE_SPEED_Z);
}

void xtouch_device_onBedTargetTempCommand(lv_msg_t *m)
{
    bambuStatus.bed_target_temper = controlMode.target_bed_temper;
    if(bambuStatus.bed_mqtt_control){
        StaticJsonDocument<256> json;json["print"]["command"]="set_bed_temp";json["print"]["temp"]=controlMode.target_bed_temper;json["print"]["sequence_id"]=xtouch_device_next_sequence();String request;serializeJson(json,request);xtouch_device_publish(request);
    }else xtouch_device_gcode_line("M140 S" + String(controlMode.target_bed_temper) + "\n");

    xtouch_device_pushall();
}

void xtouch_device_onNozzleTargetCommand(lv_msg_t *m)
{
    if(bambuStatus.nozzle_count)return;
    bambuStatus.nozzle_target_temper = controlMode.target_nozzle_temper;
    xtouch_device_gcode_line("M104 S" + String(controlMode.target_nozzle_temper) + "\n");

    xtouch_device_pushall();
}

void xtouch_device_fan(unsigned index,int value){
    if(!xtouch_bblp_fan_supported(index))return;
    if(bambuStatus.fan_new_protocol){
        StaticJsonDocument<256> json;json["print"]["command"]="set_fan";json["print"]["fan_index"]=bambuStatus.fan_protocol_ids[index];json["print"]["speed"]=(value*100+127)/255;json["print"]["sequence_id"]=xtouch_device_next_sequence();String request;serializeJson(json,request);xtouch_device_publish(request);
    }else xtouch_device_gcode_line("M106 P"+String(index+1)+" S"+String(value)+"\n");
}

void xtouch_device_onPartSpeedCommand(lv_msg_t *m)
{
    if(!xtouch_bblp_fan_supported(0))return;
    xtouch_device_fan(0,bambuStatus.cooling_fan_speed);
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onAuxSpeedCommand(lv_msg_t *m)
{
    if(!xtouch_bblp_fan_supported(1))return;
    xtouch_device_fan(1,bambuStatus.big_fan1_speed);
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onChamberSpeedCommand(lv_msg_t *m)
{
    if(!xtouch_bblp_fan_supported(2))return;
    xtouch_device_fan(2,bambuStatus.big_fan2_speed);
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onPrintSpeedCommand(lv_msg_t *m)
{
    xtouch_device_set_printing_speed(bambuStatus.printing_speed_lvl);
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onIncSwitchCommand(lv_msg_t *m)
{
    controlMode.inc = controlMode.inc == 1 ? 10 : 1;
}

void xtouch_device_onStopCommand(lv_msg_t *m)
{
    xtouch_device_publish(xtouch_device_print_action("stop"));
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onPauseCommand(lv_msg_t *m)
{
    xtouch_device_publish(xtouch_device_print_action("pause"));
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onResumeCommand(lv_msg_t *m)
{
    xtouch_device_publish(xtouch_device_print_action("resume"));
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onNozzleUp(lv_msg_t *m)
{
    if(bambuStatus.nozzle_count)return;
    xtouch_device_gcode_line("M83 \nG0 E-10.0 F900\n");
    delay(10);

    xtouch_device_pushall();
}

void xtouch_device_onNozzleDown(lv_msg_t *m)
{
    if(bambuStatus.nozzle_count)return;
    xtouch_device_gcode_line("M83 \nG0 E10.0 F900\n");
    delay(10);

    xtouch_device_pushall();
}

/* Standard single-extruder AMS/external-spool protocol, matching Bambu Studio.
   https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceManager.cpp */

static unsigned xtouch_filament_pending_printer;
static unsigned long xtouch_filament_sent_at;
static char xtouch_filament_pending_command[32];
static int xtouch_filament_legacy_tray=-1;
bool xtouch_command_pending(void){return xtouch_filament_pending_sequence.length()!=0;}
String xtouch_device_filament_command(const struct ui_filament_request &request,
    const String &sequence=xtouch_device_next_sequence())
{
    StaticJsonDocument<384> json;
    JsonObject command = json.createNestedObject("print");
    command["sequence_id"] = sequence;
    command["ams_id"] = request.tray==253?254:request.tray==254?(bambuStatus.external_multi_protocol?255:254):xtouch_ams_protocol_id(request.tray/4);
    command["slot_id"] = request.tray==254||request.tray==253?0:request.tray%4;
    if(bambuStatus.nozzle_count&&request.action!=UI_FILAMENT_REREAD)command["extruder_id"]=ui_filament_extruder(request.tray);
    if (request.action == UI_FILAMENT_REREAD) {
        command["command"] = "ams_get_rfid";
    } else {
        command["command"] = "ams_change_filament";
        int target=ui_filament_temperature(request.tray);
        int current=ui_filament_temperature(ui_filament_current(request.tray));
        /* Studio defaults to 210 C for an unload without material metadata. */
        command["curr_temp"] = current ? current : (target ? target : 210);
        command["tar_temp"] = target ? target : 210;
        command["target"] = request.action==UI_FILAMENT_UNLOAD?255:request.tray>=253?command["ams_id"].as<int>():request.tray/4>=4?xtouch_ams_protocol_id(request.tray/4):xtouch_ams_protocol_tray(request.tray);
        if (request.action == UI_FILAMENT_UNLOAD) command["slot_id"] = 255;
    }
    String result; serializeJson(json,result);
    return result;
}
void xtouch_device_onIndexedNozzle(void *,lv_msg_t *message){
 if(!message||!message->payload||xtouch_filament_pending_sequence.length())return;
 const ui_nozzle_request &r=*static_cast<const ui_nozzle_request*>(message->payload);if(!ui_nozzle_request_allowed(&r))return;
 StaticJsonDocument<256> doc;JsonObject p=doc.createNestedObject("print");xtouch_filament_pending_sequence=xtouch_device_next_sequence();xtouch_filament_pending_printer=r.printer;xtouch_filament_sent_at=millis();
 p["sequence_id"]=xtouch_filament_pending_sequence;
 if(r.action==9){p["command"]="set_ctt";p["ctt_val"]=r.target;}
 else if(r.action==0){p["command"]="set_nozzle_temp";p["extruder_index"]=r.index;p["target_temp"]=r.target;}
 else if(r.action==1){p["command"]="select_extruder";p["extruder_index"]=r.index;}
 else if(r.action==2||r.action==3){p["command"]="set_extrusion_length";p["extruder_index"]=r.index;p["length"]=r.action==2?10:-10;}
 else if(r.action==4||r.action==5){p["command"]=r.action==4?"holder_nozzle_refresh":"nozzle_info_confirm";p["id"]=r.target==255?255:r.target+16;}
 else {p["command"]="nozzle_holder_ctrl";p["action"]=r.action==6?0:r.action==7?1:2;}
 strlcpy(xtouch_filament_pending_command,p["command"]|"",sizeof(xtouch_filament_pending_command));xtouch_filament_legacy_tray=-1;
 String result;serializeJson(doc,result);if(!xtouch_device_publish(result)){xtouch_filament_pending_sequence="";ui_filament_report_send_failed();return;}ui_filament_report_status(0);
}
void xtouch_device_onAmsDry(void *,lv_msg_t *message){
 if(!message||!message->payload||xtouch_filament_pending_sequence.length())return;
 const ui_dry_request &r=*static_cast<const ui_dry_request*>(message->payload);if(!ui_ams_dry_allowed(&r))return;
 StaticJsonDocument<512> doc;JsonObject p=doc.createNestedObject("print");
 xtouch_filament_pending_sequence=xtouch_device_next_sequence();xtouch_filament_pending_printer=r.printer;xtouch_filament_sent_at=millis();
 p["sequence_id"]=xtouch_filament_pending_sequence;p["command"]="ams_filament_drying";p["ams_id"]=xtouch_ams_protocol_id(r.unit);
 p["mode"]=r.stop?0:1;p["temp"]=r.stop?0:r.temp;p["duration"]=r.stop?0:r.hours;p["humidity"]=0;p["rotate_tray"]=false;p["cooling_temp"]=r.stop?0:40;p["filament"]="";p["close_power_conflict"]=false;
 strlcpy(xtouch_filament_pending_command,p["command"]|"",sizeof(xtouch_filament_pending_command));xtouch_filament_legacy_tray=-1;
 String result;serializeJson(doc,result);if(!xtouch_device_publish(result)){xtouch_filament_pending_sequence="";ui_filament_report_send_failed();return;}ui_filament_report_status(0);
}
void xtouch_device_onFilamentAction(void *subscription, lv_msg_t *message)
{
    if (!message || !message->payload || xtouch_filament_pending_sequence.length()) return;
    const struct ui_filament_request &request = *static_cast<const struct ui_filament_request *>(message->payload);
    if (request.printer != xtouch_printer_active() || request.tray != ui_filament_selected() ||
        !ui_filament_allowed(request.action,request.tray)) return;
    static unsigned long last_command = 0;
    if (last_command && millis()-last_command < 2000) return;
    last_command = millis();
    xtouch_filament_pending_sequence=xtouch_device_next_sequence();
    xtouch_filament_pending_printer=request.printer;
    xtouch_filament_sent_at=millis();
    strlcpy(xtouch_filament_pending_command,request.action==UI_FILAMENT_REREAD?"ams_get_rfid":"ams_change_filament",sizeof(xtouch_filament_pending_command));
    xtouch_filament_legacy_tray=request.action==UI_FILAMENT_REREAD?request.tray:-1;
    if(!xtouch_device_publish(xtouch_device_filament_command(request,xtouch_filament_pending_sequence))){
        xtouch_filament_pending_sequence="";ui_filament_report_send_failed();return;
    }
    ui_filament_report_status(0);
}

void xtouch_device_filament_poll(){
 if(xtouch_filament_pending_sequence.length()&&millis()-xtouch_filament_sent_at>15000){
  xtouch_filament_pending_sequence="";if(xtouch_filament_pending_printer==xtouch_printer_active())ui_filament_report_status(2);
 }
}

void xtouch_device_filament_reply(JsonObjectConst report)
{
    /* Replies share a topic with other apps: only report this device's request. */
    if (!xtouch_filament_pending_sequence.length() ||
        xtouch_filament_pending_printer!=xtouch_printer_active()) return;
    if (millis()-xtouch_filament_sent_at>15000) {xtouch_filament_pending_sequence="";return;}
    if (report["sequence_id"].as<String>()!=xtouch_filament_pending_sequence) return;
    if(strcmp(report["command"]|"",xtouch_filament_pending_command))return;
    uint32_t error=report["err_code"]|0UL;
    XtouchReplyResult outcome=xtouch_reply_result(report["result"]|"",error);
    bool authorization=xtouch_reply_auth_failure(report["reason"]|"",error);
    if(outcome==XTOUCH_REPLY_WAIT&&!authorization)return;
    bool idle=bambuStatus.print_status==XTOUCH_PRINT_STATUS_IDLE||bambuStatus.print_status==XTOUCH_PRINT_STATUS_FINISHED||bambuStatus.print_status==XTOUCH_PRINT_STATUS_FAILED;
    bool unloaded=bambuStatus.m_tray_now==255&&!bambuStatus.tray_reading_bits&&!bambuStatus.extruder_busy;
    for(unsigned n=0;n<bambuStatus.nozzle_count&&n<2;n++)if((bambuStatus.nozzle_loaded_valid&(1U<<n))&&bambuStatus.nozzle_loaded[n]!=255)unloaded=false;
    if(outcome==XTOUCH_REPLY_REJECTED&&!strcmp(xtouch_filament_pending_command,"ams_get_rfid")&&
       xtouch_printer_online()&&!xtouch_printer_switch_pending()&&!hms_command_verification_failed()&&
       xtouch_rfid_legacy_allowed(xtouch_bblp_model(),xtouch_filament_legacy_tray,idle,unloaded,report["reason"]|"",error)){
        int tray=xtouch_filament_legacy_tray;xtouch_filament_legacy_tray=-1;
        xtouch_filament_pending_sequence=xtouch_device_next_sequence();xtouch_filament_sent_at=millis();
        strlcpy(xtouch_filament_pending_command,"gcode_line",sizeof(xtouch_filament_pending_command));
        StaticJsonDocument<256> request;request["print"]["command"]="gcode_line";
        request["print"]["sequence_id"]=xtouch_filament_pending_sequence;
        request["print"]["param"]=String("M620 R")+String(tray)+"\n";
        String payload;serializeJson(request,payload);
        if(xtouch_device_publish(payload))return;
        error=1;outcome=XTOUCH_REPLY_REJECTED;
    }
    xtouch_filament_pending_sequence="";xtouch_filament_legacy_tray=-1;
    if(outcome==XTOUCH_REPLY_REJECTED||authorization)ui_filament_report_rejection(error,authorization);else ui_filament_report_status(1);
}

void xtouch_device_command_ams_control(void *s, lv_msg_t *m)
{
    const char *action = (const char *)m->payload;

    if (
        strcmp(action, "resume") == 0 ||
        strcmp(action, "reset") == 0 ||
        strcmp(action, "pause") == 0 ||
        strcmp(action, "done") == 0)
    {
        DynamicJsonDocument json(256);
        json["print"]["command"] = "ams_control";
        json["print"]["sequence_id"] = xtouch_device_next_sequence();
        json["print"]["param"] = action;
        String result;
        serializeJson(json, result);
        xtouch_device_publish(result);
    }
}

void xtouch_device_command_clean_print_error(void *s, lv_msg_t *m)
{

    if (m->payload != NULL)
    {
        const ClearErrorMessage *message = static_cast<const ClearErrorMessage *>(m->payload);

        DynamicJsonDocument json(256);
        json["print"]["command"] = "clean_print_error";
        json["print"]["sequence_id"] = xtouch_device_next_sequence();
        json["print"]["subtask_id"] = message->subtask_id;
        json["print"]["print_error"] = message->print_error;
        String result;
        serializeJson(json, result);
        xtouch_device_publish(result);
    }
}


#endif
