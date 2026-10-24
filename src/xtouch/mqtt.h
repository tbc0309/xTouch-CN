#ifndef _XLCD_MQTT
#define _XLCD_MQTT
#include "features/diagnostics/loop_timing.h"
#include "features/diagnostics/mqtt_timing.h"
#include "features/power/runtime.h"
#include "ui/printer_models.h"
#include "ui/printer_picker.h"
#include "ui/settings_gesture.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "ui/ui_msgs.h"
#include "types.h"
#include "ams_module.h"
#include "autogrowstream.h"
// #include "xtouch/ams-status.hpp"

WiFiClientSecure xtouch_wiFiClientSecure;
PubSubClient xtouch_pubSubClient(xtouch_wiFiClientSecure);

String xtouch_mqtt_request_topic;
String xtouch_mqtt_report_topic;

#include "ams.h"
#include "device.h"
#include "config.h"

#define XTOUCH_MQTT_SERVER_TIMEOUT 20
#define XTOUCH_MQTT_SERVER_PUSH_STATUS_TIMEOUT 15
#define XTOUCH_MQTT_SERVER_JSON_PARSE_SIZE 8192

/* ---------------------------------------------- */
uint32_t xtouch_mqtt_lastPushStatus = 0;
static uint32_t xtouch_mqtt_nextAttempt = 0;
static uint32_t xtouch_mqtt_retryMs = 5000;
static bool xtouch_mqtt_hasStatus = false;

bool xtouch_printer_online(void) {
    return xtouch_pubSubClient.connected() && xtouch_mqtt_hasStatus && xtouch_printer_cloud_health(xtouch_printer_active())>=0;
}

XtouchAutoGrowBufferStream stream;

void xtouch_mqtt_sendMsg(XTOUCH_MESSAGE message, unsigned long long data = 0)
{
    XTOUCH_MESSAGE_DATA eventData;
    eventData.data = data;
    lv_msg_send(message, &eventData);
}

void xtouch_mqtt_topic_setup()
{
    String xtouch_device_topic = String("device/") + xTouchConfig.xTouchSerialNumber;
    xtouch_mqtt_request_topic = xtouch_device_topic + String("/request");
    xtouch_mqtt_report_topic = xtouch_device_topic + String("/report");
}

String xtouch_mqtt_parse_printer_type(String type_str)
{
    return String(ui_printer_model_code(type_str.c_str()));
}

// Printer payloads are external input; never let names or IDs overrun state.
template <size_t N>
static void xtouch_mqtt_copy(char (&target)[N], const char *source) {
    size_t length = strlcpy(target, source ? source : "", N);
    if (length >= N && N > 1) {
        size_t used = N - 1, start = used - 1;
        while (start && ((uint8_t)target[start] & 0xC0) == 0x80) --start;
        uint8_t lead = target[start];
        unsigned bytes = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
        if (start + bytes > used) target[start] = 0;
    }
}

void xtouch_mqtt_update_slice_info(const char *project_id, const char *profile_id, const char *subtask_id, int plate_idx)
{
    xtouch_mqtt_copy(bambuStatus.project_id_, project_id);
    xtouch_mqtt_copy(bambuStatus.profile_id_, profile_id);
    xtouch_mqtt_copy(bambuStatus.subtask_id_, subtask_id);
}

static void xtouch_mqtt_set_model(const char *code){
 if(!code||!*code||!strcmp(code,bambuStatus.printer_type))return;
 xtouch_mqtt_copy(bambuStatus.printer_type,code);
 if(!strcmp(code,"C11")||!strcmp(code,"C12")){bambuStatus.chamber_valid=false;bambuStatus.chamber_temper=0;xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP,0);}
 bambuStatus.nozzle_count=bambuStatus.nozzle_valid=bambuStatus.nozzle_loaded_valid=0;
 memset(bambuStatus.ams_bound_extruder,255,sizeof(bambuStatus.ams_bound_extruder));
 xtouch_filament_pending_sequence="";xtouch_device_control_clear();
 xtouch_mqtt_sendMsg(XTOUCH_ON_MODEL);
}

#include "ams_report.h"

// LAN and Cloud enter the same parser. Missing delta fields preserve state;
// explicit malformed values never turn into a plausible zero reading.
static bool xtouch_mqtt_number(JsonVariantConst value,double low,double high,double &out){
 if(value.is<const char*>()){const char *s=value.as<const char*>();if(!*s)return false;char *end;out=strtod(s,&end);if(*end)return false;}
 else if(value.is<double>()||value.is<long>()||value.is<unsigned long>())out=value.as<double>();else return false;
 return isfinite(out)&&out>=low&&out<=high;
}
void xtouch_mqtt_processPushStatus(JsonDocument &incomingJson)
{
    bool faultsChanged=false;
    xtouch_mqtt_lastPushStatus = millis();
    xtouch_mqtt_hasStatus = true;
    ConsoleDebug.println(F("[XTouch][MQTT] ProcessPushStatus"));

    if (incomingJson != NULL && incomingJson.containsKey("print"))
    {
        JsonObjectConst telemetry=incomingJson["print"].as<JsonObjectConst>();
        if(telemetry["support_chamber_temp_display"].is<bool>()){bambuStatus.chamber_support_known=true;bambuStatus.chamber_supported=telemetry["support_chamber_temp_display"];}
        else if(telemetry["support_chamber"].is<bool>()){bambuStatus.chamber_support_known=true;bambuStatus.chamber_supported=telemetry["support_chamber"];}
        if(telemetry["support_chamber_temp_edit"].is<bool>()){bambuStatus.chamber_edit_known=true;bambuStatus.chamber_edit_supported=telemetry["support_chamber_temp_edit"];}
        if(telemetry["support_chamber_temp_edit_range"].is<JsonArrayConst>()){double low,high;JsonArrayConst range=telemetry["support_chamber_temp_edit_range"];if(range.size()==2&&xtouch_mqtt_number(range[0],0,100,low)&&xtouch_mqtt_number(range[1],low,100,high)){bambuStatus.chamber_edit_min=low;bambuStatus.chamber_edit_max=high;}}
        const char *fanFlags[]={"support_aux_fan","support_chamber_fan"};for(unsigned i=0;i<2;i++)if(telemetry[fanFlags[i]].is<bool>()){bambuStatus.fan_support_known|=1U<<(i+1);if(telemetry[fanFlags[i]].as<bool>())bambuStatus.fan_supported|=1U<<(i+1);else bambuStatus.fan_supported&=~(1U<<(i+1));}
        if(telemetry["device"]["airduct"].is<JsonObjectConst>())bambuStatus.fan_new_protocol=true;
        if(telemetry.containsKey("support_aux_fan")||telemetry.containsKey("support_chamber_fan")||telemetry["device"]["airduct"].is<JsonObjectConst>()){xtouch_mqtt_sendMsg(XTOUCH_ON_PART_FAN_SPEED,bambuStatus.cooling_fan_speed);xtouch_mqtt_sendMsg(XTOUCH_ON_PART_AUX_SPEED,bambuStatus.big_fan1_speed);xtouch_mqtt_sendMsg(XTOUCH_ON_PART_CHAMBER_SPEED,bambuStatus.big_fan2_speed);}
        // #pragma region printing

        if (incomingJson["print"].containsKey("print_type"))
        {
            xtouch_mqtt_copy(bambuStatus.print_type, incomingJson["print"]["print_type"]);
        }
        if (incomingJson["print"].containsKey("home_flag"))
        {
            bambuStatus.home_flag = incomingJson["print"]["home_flag"].as<int>();
        }

        if (incomingJson["print"].containsKey("hw_switch_state"))
        {
            bambuStatus.hw_switch_state = incomingJson["print"]["hw_switch_state"].as<int>();
        }

        if(incomingJson["print"].containsKey("mc_remaining_time")){double value;if(xtouch_mqtt_number(incomingJson["print"]["mc_remaining_time"],0,525600,value))bambuStatus.mc_left_time=(int)value*60;}
        if(incomingJson["print"].containsKey("mc_percent")){double value;if(xtouch_mqtt_number(incomingJson["print"]["mc_percent"],0,100,value))bambuStatus.mc_print_percent=value;}
        if (incomingJson["print"].containsKey("mc_print_sub_stage"))
        {
            bambuStatus.mc_print_sub_stage = incomingJson["print"]["mc_print_sub_stage"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_stage"))
        {
            if (incomingJson["print"]["mc_print_stage"].is<String>())
                bambuStatus.mc_print_stage = atoi(incomingJson["print"]["mc_print_stage"].as<String>().c_str());
            if (incomingJson["print"]["mc_print_stage"].is<int>())
                bambuStatus.mc_print_stage = incomingJson["print"]["mc_print_stage"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_error_code"))
        {
            if (incomingJson["print"]["mc_print_error_code"].is<int>())
                bambuStatus.mc_print_error_code = incomingJson["print"]["mc_print_error_code"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_line_number"))
        {
            if (incomingJson["print"]["mc_print_line_number"].is<String>())
            {
                String mc_print_line_number_str = incomingJson["print"]["mc_print_line_number"].as<String>();
                if (mc_print_line_number_str != "")
                {
                    bambuStatus.mc_print_line_number = atoi(mc_print_line_number_str.c_str());
                }
            }
        }

        if (incomingJson["print"].containsKey("print_error"))
        {
            if (incomingJson["print"]["print_error"].is<uint32_t>())
            {
                uint32_t code=incomingJson["print"]["print_error"].as<uint32_t>();
                bambuStatus.print_error=(int)code;
                faultsChanged|=hms_set_print_error(code);
            }
        }

        // #pragma endregion

        // #pragma region online
        // #pragma endregion

        // #pragma region print_task
        if (incomingJson["print"].containsKey("printer_type"))
        {
            String code = xtouch_mqtt_parse_printer_type(incomingJson["print"]["printer_type"].as<String>());
            if (code.length() && code != bambuStatus.printer_type) {
                xtouch_mqtt_set_model(code.c_str());
            }
        }

        if (incomingJson["print"].containsKey("subtask_name"))
        {
            xtouch_mqtt_copy(bambuStatus.subtask_name, incomingJson["print"]["subtask_name"]);
        }

        if (incomingJson["print"].containsKey("layer_num"))
        {
            bambuStatus.current_layer = incomingJson["print"]["layer_num"].as<int>();
        }

        if (incomingJson["print"].containsKey("total_layer_num"))
        {
            bambuStatus.total_layers = incomingJson["print"]["total_layer_num"].as<int>();
        }

        if (incomingJson["print"].containsKey("gcode_state"))
        {
            xtouch_device_set_print_state(incomingJson["print"]["gcode_state"].as<String>());
        }

        if (incomingJson["print"].containsKey("queue_number"))
        {
            bambuStatus.queue_number = incomingJson["print"]["queue_number"].as<int>();
        }

        if (incomingJson["print"].containsKey("task_id"))
        {
            String task = incomingJson["print"]["task_id"].as<String>();
            if(strcmp(bambuStatus.task_id,task.c_str())){
                bambuStatus.obj_subtask_id[0]=0;
                bambuStatus.plate_index=1;
            }
            xtouch_mqtt_copy(bambuStatus.task_id, task.c_str());
        }

        if (incomingJson["print"].containsKey("gcode_file"))
        {
            if(strcmp(bambuStatus.gcode_file,incomingJson["print"]["gcode_file"]|""))bambuStatus.plate_index=1;
            xtouch_mqtt_copy(bambuStatus.gcode_file, incomingJson["print"]["gcode_file"]);
        }
        if(incomingJson["print"].containsKey("subtask_id")){
            String subtask=incomingJson["print"]["subtask_id"].as<String>();
            if(strcmp(bambuStatus.obj_subtask_id,subtask.c_str()))bambuStatus.plate_index=1;
            xtouch_mqtt_copy(bambuStatus.obj_subtask_id,subtask.c_str());
        }
        for(const char *plateKey:{"plate_idx","plate_index"}){
            if(incomingJson["print"][plateKey].is<unsigned>()){
                unsigned plate=incomingJson["print"][plateKey];if(plate>0&&plate<=999)bambuStatus.plate_index=plate;
            }
        }

        if (incomingJson["print"].containsKey("gcode_file_prepare_percent"))
        {
            String percent_str = incomingJson["print"]["gcode_file_prepare_percent"].as<String>();
            if (percent_str != "")
            {
                bambuStatus.gcode_file_prepare_percent = atoi(percent_str.c_str());
            }
        }

        if (incomingJson["print"].containsKey("project_id") && incomingJson["print"].containsKey("profile_id") && incomingJson["print"].containsKey("subtask_id"))
        {
            String obj_subtask_id_string = incomingJson["print"]["subtask_id"].as<String>();
            xtouch_mqtt_copy(bambuStatus.obj_subtask_id, obj_subtask_id_string.c_str());

            int plate_index = -1;
            /* parse local plate_index from task */
            if (obj_subtask_id_string == "0" && incomingJson["print"]["profile_id"].as<String>() != "0")
            {
                if (incomingJson["print"].containsKey("gcode_file"))
                {
                    String gcode_file_string = incomingJson["print"]["gcode_file"].as<String>();
                    xtouch_mqtt_copy(bambuStatus.gcode_file, incomingJson["print"]["gcode_file"]);

                    int idx_start = gcode_file_string.lastIndexOf("_") + 1;
                    int idx_end = gcode_file_string.lastIndexOf(".");
                    if (gcode_file_string.startsWith("Metadata/plate_") && idx_start > 0 && idx_end > idx_start && !incomingJson["print"].containsKey("plate_idx") && !incomingJson["print"].containsKey("plate_index"))
                    {
                        plate_index = atoi(gcode_file_string.substring(idx_start, idx_end).c_str());
                        if(plate_index>0 && plate_index<=999)bambuStatus.plate_index = plate_index;
                    }
                }
            }
            xtouch_mqtt_update_slice_info(incomingJson["print"]["project_id"], incomingJson["print"]["profile_id"], incomingJson["print"]["subtask_id"], plate_index);

            if(!incomingJson["print"].containsKey("task_id"))xtouch_mqtt_copy(bambuStatus.task_id, incomingJson["print"]["subtask_id"]);
        }
        // #pragma region print_task

        // #pragma region status

        const char *temperatureKeys[]={"bed_temper","bed_target_temper","nozzle_temper","nozzle_target_temper","frame_temper"};
        double *temperatures[]={&bambuStatus.bed_temper,&bambuStatus.bed_target_temper,&bambuStatus.nozzle_temper,&bambuStatus.nozzle_target_temper,&bambuStatus.frame_temp};
        const unsigned temperatureEvents[]={XTOUCH_ON_BED_TEMP,XTOUCH_ON_BED_TARGET_TEMP,XTOUCH_ON_NOZZLE_TEMP,XTOUCH_ON_NOZZLE_TARGET_TEMP,0};
        for(unsigned i=0;i<5;i++)if(incomingJson["print"].containsKey(temperatureKeys[i])){
            double value;bool valid=xtouch_mqtt_number(incomingJson["print"][temperatureKeys[i]],i==4?-20:0,i<2?160:i<4?400:200,value);
            if(valid){bambuStatus.temperature_valid|=1U<<i;*temperatures[i]=value;}else bambuStatus.temperature_valid&=~(1U<<i);
            if(temperatureEvents[i])xtouch_mqtt_sendMsg((XTOUCH_MESSAGE)temperatureEvents[i],valid?value:0);
        }
        if(incomingJson["print"].containsKey("chamber_temper")&&xtouch_bblp_network_chamber()){
            double value;bambuStatus.chamber_valid=xtouch_mqtt_number(incomingJson["print"]["chamber_temper"],-20,200,value);
            if(bambuStatus.chamber_valid)bambuStatus.chamber_temper=value;
            xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP,bambuStatus.chamber_temper);
        }
        // BambuStudio DevChamber: legacy target and newer packed CTC telemetry.
        if (incomingJson["print"]["ctt"].is<int>()){
            int target=incomingJson["print"]["ctt"];bambuStatus.chamber_target_valid=target>=0&&target<=100;
            if(bambuStatus.chamber_target_valid)bambuStatus.chamber_target_temper=target;
        }
        JsonVariant packed = incomingJson["print"]["device"]["ctc"]["info"]["temp"];
        if (xtouch_bblp_network_chamber() && (packed.is<uint32_t>() || packed.is<int32_t>())) {
            uint32_t value = packed.as<uint32_t>();
            unsigned actual = value & 0xFFFF, target = value >> 16;
            // Explicit unavailable values invalidate stale readings; absent deltas preserve them.
            bambuStatus.chamber_valid=actual!=0xFFFF&&actual<=200;
            if (actual != 0xFFFF && actual <= 200) {
                bambuStatus.chamber_valid=true;bambuStatus.chamber_temper = actual;
                xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP, actual);
            }
            bambuStatus.chamber_target_valid=target!=0xFFFF&&target<=100;
            if (bambuStatus.chamber_target_valid)bambuStatus.chamber_target_temper=target;
        }

        if(!xtouch_bblp_chamber_display()){bambuStatus.chamber_valid=false;xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP,0);}
        // link_th
        // link_ams
        if (incomingJson["print"].containsKey("wifi_signal"))
        {
            String wifi_signal = incomingJson["print"]["wifi_signal"].as<String>();
            wifi_signal.replace("dBm", "");
            char *end;long strength=strtol(wifi_signal.c_str(),&end,10);
            if(wifi_signal.length()&&!*end&&strength>=-120&&strength<=0){bambuStatus.wifi_signal=-strength;bambuStatus.wifi_signal_valid=true;xtouch_mqtt_sendMsg(XTOUCH_ON_WIFI_SIGNAL,bambuStatus.wifi_signal);}
        }

        double fanPacked;
        if(incomingJson["print"].containsKey("fan_gear")&&xtouch_mqtt_number(incomingJson["print"]["fan_gear"],0,4294967295.0,fanPacked)&&floor(fanPacked)==fanPacked){
            uint32_t bits=(uint32_t)fanPacked;bambuStatus.cooling_fan_speed=bits&255;bambuStatus.big_fan1_speed=(bits>>8)&255;bambuStatus.big_fan2_speed=(bits>>16)&255;bambuStatus.fan_valid=7;
            xtouch_mqtt_sendMsg(XTOUCH_ON_PART_FAN_SPEED,bambuStatus.cooling_fan_speed);xtouch_mqtt_sendMsg(XTOUCH_ON_PART_AUX_SPEED,bambuStatus.big_fan1_speed);xtouch_mqtt_sendMsg(XTOUCH_ON_PART_CHAMBER_SPEED,bambuStatus.big_fan2_speed);
        }else{
            if(incomingJson["print"].containsKey("fan_gear")){bambuStatus.fan_valid=0;xtouch_mqtt_sendMsg(XTOUCH_ON_PART_FAN_SPEED,0);xtouch_mqtt_sendMsg(XTOUCH_ON_PART_AUX_SPEED,0);xtouch_mqtt_sendMsg(XTOUCH_ON_PART_CHAMBER_SPEED,0);}
            const char *keys[]={"cooling_fan_speed","big_fan1_speed","big_fan2_speed"};int *values[]={&bambuStatus.cooling_fan_speed,&bambuStatus.big_fan1_speed,&bambuStatus.big_fan2_speed};const unsigned messages[]={XTOUCH_ON_PART_FAN_SPEED,XTOUCH_ON_PART_AUX_SPEED,XTOUCH_ON_PART_CHAMBER_SPEED};
            for(unsigned i=0;i<3;i++)if(incomingJson["print"].containsKey(keys[i])){double value;bool valid=xtouch_mqtt_number(incomingJson["print"][keys[i]],0,15,value);if(valid){*values[i]=round(floor(value/1.5)*25.5);bambuStatus.fan_valid|=1U<<i;}else bambuStatus.fan_valid&=~(1U<<i);xtouch_mqtt_sendMsg((XTOUCH_MESSAGE)messages[i],valid?*values[i]:0);}
        }
        // heatbreak_fan_speed

        if (incomingJson["print"].containsKey("spd_lvl"))
        {
            bambuStatus.printing_speed_lvl = incomingJson["print"]["spd_lvl"].as<int>();
        }

        if (incomingJson["print"].containsKey("spd_mag"))
        {
            bambuStatus.printing_speed_mag = incomingJson["print"]["spd_mag"].as<int>();
        }

        // stg
        // stg_cur
        // filam_bak
        // mess_production_state
        // lifecycle

        if (incomingJson["print"].containsKey("lights_report"))
        {

            if (incomingJson["print"]["lights_report"][0].containsKey("mode"))
            {
                XTOUCH_MESSAGE_DATA eventData;
                if (incomingJson["print"]["lights_report"][0]["mode"] == "on")
                {
                    bambuStatus.chamberLed = true;
                    eventData.data = 1;
                }
                else
                {
                    bambuStatus.chamberLed = false;
                    eventData.data = 0;
                }
                lv_msg_send(XTOUCH_ON_LIGHT_REPORT, &eventData);
            }
        }

        // sdcard

        // #pragma endregion

        if (incomingJson["print"].containsKey("nozzle_diameter"))
        {
            if (incomingJson["print"]["nozzle_diameter"].is<float>())
            {
                bambuStatus.nozzle_diameter = incomingJson["print"]["nozzle_diameter"].as<float>();
            }
            else if (incomingJson["print"]["nozzle_diameter"].is<String>())
            {
                bambuStatus.nozzle_diameter = incomingJson["print"]["nozzle_diameter"].as<String>().toFloat();
            }
        }

        // #pragma region upgrade
        // #pragma endregion

        // #pragma region  camera
        if (incomingJson["print"].containsKey("ipcam"))
        {

            if (incomingJson["print"]["ipcam"].containsKey("ipcam_record"))
            {
                bambuStatus.camera_recording_when_printing = incomingJson["ipcam"]["ipcam_record"].as<String>() == "enable";
            }
            if (incomingJson["print"]["ipcam"].containsKey("timelapse"))
            {
                bambuStatus.camera_timelapse = incomingJson["print"]["ipcam"]["timelapse"].as<String>() == "enable";
            }
            if (incomingJson["print"]["ipcam"].containsKey("ipcam_dev"))
            {
                bambuStatus.has_ipcam = incomingJson["print"]["ipcam"]["ipcam_dev"].as<String>() == "1";
            }
            xtouch_mqtt_sendMsg(XTOUCH_ON_IPCAM);
        }

        // xcam

        // #pragma endregion

        // #pragma region hms
        if (incomingJson["print"].containsKey("hms"))
        {
            if (incomingJson["print"]["hms"].is<JsonArray>())
            {

                uint64_t codes[HMS_MAX_CODES];unsigned count=0;bool valid=true,overflow=false;
                for (JsonVariant value : incomingJson["print"]["hms"].as<JsonArray>())
                {
                    JsonObject element=value.as<JsonObject>();
                    if(!element["attr"].is<uint32_t>()||!element["code"].is<uint32_t>()){valid=false;continue;}
                    // Preserve both raw words, including unknown future module/level bits.
                    uint64_t code=((uint64_t)element["attr"].as<uint32_t>()<<32)|element["code"].as<uint32_t>();
                    if(!code||code==UINT64_MAX)continue;
                    bool duplicate=false;for(unsigned i=0;i<count;++i)if(codes[i]==code){duplicate=true;break;}
                    if(duplicate)continue;
                    if(count<HMS_MAX_CODES)codes[count++]=code;
                    else{overflow=true;unsigned largest=0;for(unsigned i=1;i<count;++i)if(codes[i]>codes[largest])largest=i;if(code<codes[largest])codes[largest]=code;}
                }
                if(valid)faultsChanged|=hms_set_health(codes,count,overflow);
            }
        }

        // #pragma endregion

        if(incomingJson["print"]["device"].containsKey("type"))bambuStatus.non_fdm_mode=!(incomingJson["print"]["device"]["type"].as<unsigned>()&1);
        JsonObjectConst extruders=incomingJson["print"]["device"]["extruder"].as<JsonObjectConst>();
        if(extruders.containsKey("state")){
            unsigned state=extruders["state"] | 0U;unsigned count=state&15;bambuStatus.extruder_busy=((state>>12)&7)||((state>>19)&1);
            if(count>=1&&count<=2){bambuStatus.nozzle_count=count;bambuStatus.active_nozzle=((state>>4)&15)<count?(state>>4)&15:255;if(count==1){bambuStatus.nozzle_valid&=1;bambuStatus.nozzle_loaded_valid&=1;}}
            else {bambuStatus.nozzle_count=0;bambuStatus.nozzle_valid=0;bambuStatus.nozzle_loaded_valid=0;}
        }
        for(JsonObjectConst nozzle:extruders["info"].as<JsonArrayConst>()){
            if(!nozzle["id"].is<unsigned>())continue;unsigned id=nozzle["id"];
            if(id>=2)continue;
            if(nozzle["temp"].is<uint32_t>()){
                uint32_t packed=nozzle["temp"];unsigned actual=packed&0xffff,target=packed>>16;
                if(actual<=400&&target<=350){bambuStatus.nozzle_actual[id]=actual;bambuStatus.nozzle_target[id]=target;bambuStatus.nozzle_valid|=1U<<id;}
                else bambuStatus.nozzle_valid&=~(1U<<id);
            }
            if(nozzle["snow"].is<unsigned>()){
                unsigned packed=nozzle["snow"];int ams=(packed>>8)&255,slot=packed&255;
                bambuStatus.nozzle_loaded_valid|=1U<<id;int unit=xtouch_ams_unit_index(ams);bambuStatus.nozzle_loaded[id]=packed==0xffff?255:unit>=0&&slot<(int)xtouch_ams_slot_count(unit)?unit*4+slot:(ams==255&&slot==0)?254:(ams==254&&slot==0)?(bambuStatus.nozzle_count==1&&!bambuStatus.external_multi_protocol?254:253):-1;
            }
        }
        if(bambuStatus.nozzle_count&&bambuStatus.active_nozzle<2&&(bambuStatus.nozzle_valid&(1U<<bambuStatus.active_nozzle))){
            bambuStatus.temperature_valid|=12;
            bambuStatus.nozzle_temper=bambuStatus.nozzle_actual[bambuStatus.active_nozzle];bambuStatus.nozzle_target_temper=bambuStatus.nozzle_target[bambuStatus.active_nozzle];
            xtouch_mqtt_sendMsg(XTOUCH_ON_NOZZLE_TEMP,bambuStatus.nozzle_temper);xtouch_mqtt_sendMsg(XTOUCH_ON_NOZZLE_TARGET_TEMP,bambuStatus.nozzle_target_temper);
        }
        if(bambuStatus.nozzle_count&&(bambuStatus.active_nozzle>=2||!(bambuStatus.nozzle_valid&(1U<<bambuStatus.active_nozzle)))){bambuStatus.temperature_valid&=~12;xtouch_mqtt_sendMsg(XTOUCH_ON_NOZZLE_TEMP,0);xtouch_mqtt_sendMsg(XTOUCH_ON_NOZZLE_TARGET_TEMP,0);}
        JsonObjectConst device=incomingJson["print"]["device"].as<JsonObjectConst>();
        JsonObjectConst holder=device["holder"].as<JsonObjectConst>();
        if(holder["stat"].is<unsigned>()){
            unsigned stat=holder["stat"];bambuStatus.rack_present=true;
            bambuStatus.rack_status=stat<10?stat:255;
        }
        if(holder["pos"].is<unsigned>()){unsigned pos=holder["pos"];bambuStatus.rack_position=pos<=3?pos:0;}
        JsonObjectConst heads=device["nozzle"].as<JsonObjectConst>();
        if(heads["state"].is<unsigned>())bambuStatus.rack_reading=(heads["state"].as<unsigned>()>>4)&15;
        if(heads["info"].is<JsonArrayConst>()){
            bambuStatus.rack_mask=0;
            for(JsonObjectConst n:heads["info"].as<JsonArrayConst>()){
                if(!n["id"].is<unsigned>())continue;unsigned packed=n["id"],id=packed&15;
                if(((packed>>4)&15)!=1)continue;
                double diameter=n["diameter"]|0.0;
                if(!isfinite(diameter)||diameter<=0||diameter>2)continue;
                bambuStatus.rack_mask|=1U<<id;bambuStatus.rack_diameter[id]=lround(diameter*10);
                snprintf(bambuStatus.rack_type[id],sizeof(bambuStatus.rack_type[id]),"%s",n["type"]|"?");
            }
        }
        xtouch_mqtt_parse_ams(incomingJson);
        if (incomingJson["print"].containsKey("ams_status")) {
            xtouch_ams_parse_status(incomingJson["print"]["ams_status"].as<int>());
            xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS, 0);
        }

        if(incomingJson["print"]["vir_slot"].is<JsonArrayConst>()){
            bambuStatus.external_multi_protocol=true;
            if(incomingJson["print"]["vir_slot"].size()==0){bambuStatus.external_routes=0;bambuStatus.external_materials=0;}
            for(JsonObjectConst ext:incomingJson["print"]["vir_slot"].as<JsonArrayConst>()){
                const char *idText=ext["id"].is<const char*>()?ext["id"].as<const char*>():NULL;char *idEnd=NULL;long id=idText?strtol(idText,&idEnd,10):ext["id"]|0;
                if(idText&&(!*idText||!idEnd||*idEnd))continue;
                if(id!=254&&id!=255)continue;unsigned channel=id==255?0:1;
                bambuStatus.external_routes|=1U<<channel;
                if(ext.containsKey("tray_type")){
                    if(strlen(ext["tray_type"]|""))bambuStatus.external_materials|=1U<<channel;else bambuStatus.external_materials&=~(1U<<channel);
                }
                const char *color=ext["tray_color"]|"";bool valid=strlen(color)==8;for(unsigned n=0;valid&&n<8;n++)valid=isxdigit(color[n]);
                if(valid){uint32_t rgb=strtoul(color,NULL,16)>>8;if(channel)bambuStatus.external_color2=rgb;else bambuStatus.external_color=rgb;}
                auto temp=[](JsonVariantConst v)->unsigned{const char *s=v.is<const char*>()?v.as<const char*>():NULL;char *end=NULL;long n=s?strtol(s,&end,10):v.is<int>()?v.as<int>():0;return (!s||(*s&&end&&!*end))&&n>=150&&n<=350?n:0;};
                if(ext.containsKey("nozzle_temp_min")){if(channel)bambuStatus.external_nozzle_min2=temp(ext["nozzle_temp_min"]);else bambuStatus.external_nozzle_min=temp(ext["nozzle_temp_min"]);}
                if(ext.containsKey("nozzle_temp_max")){if(channel)bambuStatus.external_nozzle_max2=temp(ext["nozzle_temp_max"]);else bambuStatus.external_nozzle_max=temp(ext["nozzle_temp_max"]);}
            }
            bambuStatus.external_present=bambuStatus.external_materials!=0;xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS,0);
        }
        // vt_tray
        if (!bambuStatus.external_multi_protocol && incomingJson["print"].containsKey("vt_tray"))
        {
            bambuStatus.ams_support_virtual_tray = true;
            JsonObjectConst external=incomingJson["print"]["vt_tray"].as<JsonObjectConst>();
            if(external.isNull() || external.size()==0)bambuStatus.external_present=false;
            if(external.containsKey("tray_type")) bambuStatus.external_present=strlen(external["tray_type"] | "")>0;
            if(external.containsKey("tray_color")) {
                const char *hex=external["tray_color"] | "";bool valid=strlen(hex)==8;
                for(unsigned i=0;valid && i<8;++i) valid=isxdigit(hex[i]);
                bambuStatus.external_color=valid ? strtoul(hex,nullptr,16)>>8 : 0x777777;
            }
            auto temperature=[](JsonVariantConst v)->uint16_t {
                long value=0;
                if (v.is<const char *>()) {
                    const char *text=v.as<const char *>(); char *end;
                    value=strtol(text,&end,10); if(end==text || *end) return 0;
                } else if(v.is<int>()) value=v.as<int>();
                return value>=150 && value<=350 ? value : 0;
            };
            if(external.containsKey("nozzle_temp_min")) bambuStatus.external_nozzle_min=temperature(external["nozzle_temp_min"]);
            if(external.containsKey("nozzle_temp_max")) bambuStatus.external_nozzle_max=temperature(external["nozzle_temp_max"]);
            xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS,0);
        }
        else
        {
            // An omitted delta is not a capability removal.
        }
        // #pragma endregion

        if(faultsChanged){
            ConsoleInfo.printf("[HMS] report print=0x%08lX count=%u pending=%u\n",(unsigned long)hms_print_error(),hms_health_count(),(unsigned)hms_has_pending());
            for(unsigned i=0;i<hms_health_count();++i){uint64_t code=hms_health_at(i);ConsoleInfo.printf("[HMS] code=%08lX%08lX\n",(unsigned long)(code>>32),(unsigned long)code);}
            xtouch_mqtt_sendMsg(XTOUCH_ON_ERROR,0);
        }
        if (incomingJson["print"].containsKey("gcode_state") ||
            incomingJson["print"].containsKey("layer_num") ||
            incomingJson["print"].containsKey("total_layer_num") ||
            incomingJson["print"].containsKey("mc_remaining_time") ||
            incomingJson["print"].containsKey("mc_percent") ||
            incomingJson["print"].containsKey("spd_lvl") ||
            incomingJson["print"].containsKey("spd_mag"))
        {

            xtouch_mqtt_sendMsg(XTOUCH_ON_PRINT_STATUS);
        }
    }
}

void xtouch_mqtt_parseMessage(XtouchAutoGrowBufferStream &input, byte type = 0)
{

    ConsoleDebug.println(F("[XTouch][MQTT] ParseMessage"));
    DynamicJsonDocument incomingJson(type==1?12288:XTOUCH_MQTT_SERVER_JSON_PARSE_SIZE);
    DynamicJsonDocument amsFilter(2048);
    if(type==0){
        for(const char *key:{"support_chamber","support_chamber_temp_display","support_chamber_temp_edit","support_chamber_temp_edit_range","support_aux_fan","support_chamber_fan","sequence_id","err_code","result","reason","vir_slot","ams_status","bed_target_temper","bed_temper","big_fan1_speed","big_fan2_speed","chamber_temper","command","cooling_fan_speed","ctt","fan_gear","frame_temper","gcode_file","plate_idx","plate_index","gcode_file_prepare_percent","gcode_state","hms","home_flag","hw_switch_state","ipcam","layer_num","lights_report","mc_percent","mc_print_error_code","mc_print_line_number","mc_print_stage","mc_print_sub_stage","mc_remaining_time","nozzle_diameter","nozzle_target_temper","nozzle_temper","print_error","print_type","printer_type","profile_id","project_id","queue_number","spd_lvl","spd_mag","subtask_id","subtask_name","task_id","total_layer_num","vt_tray","wifi_signal"})amsFilter["print"][key]=true;
        amsFilter["print"]["device"]["type"]=true;amsFilter["print"]["device"]["airduct"]["modeCur"]=true;
        for(const char *key:{"extruder","ctc","holder"})amsFilter["print"]["device"][key]=true;
        amsFilter["print"]["device"]["nozzle"]["state"]=true;
        JsonObject headFilter=amsFilter["print"]["device"]["nozzle"]["info"].to<JsonArray>().createNestedObject();
        for(const char *key:{"id","diameter","type"})headFilter[key]=true;
        amsFilter["camera"]["command"]=true;amsFilter["camera"]["control"]=true;
        for(const char *key:{"command","sequence_id","err_code","result","reason"})amsFilter["system"][key]=true;
        amsFilter["info"]["module"][0]["name"]=true;amsFilter["info"]["module"][0]["product_name"]=true;
    }else{
        amsFilter["print"]["command"]=true;
        JsonObject filter=amsFilter["print"].createNestedObject("ams");
        for(const char *key:{"ams_exist_bits","tray_exist_bits","tray_now","tray_tar","version","tray_is_bbl_bits","tray_read_done_bits","tray_reading_bits"})filter[key]=true;
        JsonObject unit=filter.createNestedArray("ams").createNestedObject();
        for(const char *key:{"id","info","temp","humidity","humidity_raw","dry_time","dry_setting"})unit[key]=true;
        JsonObject tray=unit.createNestedArray("tray").createNestedObject();
        for(const char *key:{"id","remain","tray_color","tray_type","nozzle_temp_min","nozzle_temp_max"})tray[key]=true;
    }

    if(amsFilter.overflowed()){Serial.println("[MQTT] receive filter overflow");return;}
    input.rewind();
    auto deserializeError = deserializeJson(incomingJson, input, DeserializationOption::Filter(amsFilter));

    // xtouch_debug_json(incomingJson);
    if (!deserializeError)
    {
        // get_version is already requested on connect. Only OTA identifies the
        // printer; an AMS/accessory product name cannot replace its model.
        bool accessoryChanged=false;
        for (JsonObject module : incomingJson["info"]["module"].as<JsonArray>()) {
            const char *name = module["name"] | "";
            int raw=-1;unsigned kind=xtouch_ams_module_kind(name,&raw);
            int index=xtouch_ams_unit_index(raw);
            if(kind&&index>=0){bambuStatus.ams_module_kind[index]=kind;if(!bambuStatus.ams_kind[index]){bambuStatus.ams_kind[index]=kind;accessoryChanged=true;}}
            if (strcmp(name, "ota")) continue;
            const char *code = ui_printer_model_code(module["product_name"] | "");
            if (code[0] && strcmp(code, bambuStatus.printer_type)) {
                xtouch_mqtt_set_model(code);
            }
        }
        if(accessoryChanged)xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS,0);
        if ((millis() - xtouch_mqtt_lastPushStatus) > ((xtouch_printer_cloud()?180U:XTOUCH_MQTT_SERVER_PUSH_STATUS_TIMEOUT) * 1000U))
        {
            Serial.println(F("[XTouch][MQTT] Reconnect after status timeout"));
            xtouch_pubSubClient.disconnect();
        }

        if (incomingJson["system"].is<JsonObjectConst>())
            xtouch_device_control_reply(incomingJson["system"].as<JsonObjectConst>());
        if (incomingJson.containsKey("print") && incomingJson["print"].containsKey("command"))
        {

            xtouch_device_control_reply(incomingJson["print"].as<JsonObjectConst>());
            String command = incomingJson["print"]["command"].as<String>();

            if (command == "push_status")
            {
                xtouch_mqtt_processPushStatus(incomingJson);
            }
            else if (xtouch_device_scoped_command(command.c_str())) {
                xtouch_device_filament_reply(incomingJson["print"].as<JsonObjectConst>());
            }
            else if (command == "gcode_line")
            {
                xtouch_device_filament_reply(incomingJson["print"].as<JsonObjectConst>());
                ConsoleDebug.println(F("[XTouch][MQTT] gcode_line ack"));
            }

            // project_file
            // ams_filament_setting
            // xcam_control_set
            // print_option
            // extrusion_cali | flowrate_cali
            // extrusion_cali_set
            // extrusion_cali_sel
            // extrusion_cali_get
            // extrusion_cali_get_result
            // flowrate_get_result
        }

        // info

        if (incomingJson.containsKey("camera"))
        {
            if (incomingJson["camera"].containsKey("command"))
            {
                if (incomingJson["camera"]["command"].as<String>() == "ipcam_timelapse")
                {
                    bambuStatus.camera_timelapse = incomingJson["camera"]["control"].as<String>() == "enable";
                }
                else if (incomingJson["camera"]["command"].as<String>() == "ipcam_record_set")
                {
                    bambuStatus.camera_recording_when_printing = incomingJson["camera"]["control"].as<String>() == "enable";
                }
                xtouch_mqtt_sendMsg(XTOUCH_ON_IPCAM);
            }
        }

        // upgrade
        // event info
    }
    else
    {
        Serial.printf("[MQTT] JSON parse failed: %s, capacity=%u\n",deserializeError.c_str(),unsigned(incomingJson.capacity()));
    }


}

void xtouch_pubSubClient_streamCallback(char *topic, byte *payload, unsigned int length)
{
    static bool reported=false;if(!reported){Serial.printf("[MQTT] receive bytes=%u stored=%u failed=%d topicMatch=%d pending=%d\n",length,unsigned(stream.current_length()),stream.failed(),strcmp(topic,xtouch_mqtt_report_topic.c_str())==0,xtouch_printer_switch_pending());reported=true;}
    // Discard late packets from the previous device after a switch.
    if (stream.failed() || strcmp(topic, xtouch_mqtt_report_topic.c_str()) != 0 || xtouch_printer_switch_pending()) {
        stream.flush();
        return;
    }
    static bool firstFrame=true;if(firstFrame){Serial.printf("[MQTT] first frame bytes=%u\n",unsigned(length));firstFrame=false;}
    bool includesAms=stream.includes("\"ams\"");
    xtouch_mqtt_parseMessage(stream, 0);

    if (includesAms)
    {
        xtouch_mqtt_parseMessage(stream, 1);
    }

    stream.flush();
}

const char *xtouch_mqtt_generateRandomKey(int keyLength)
{
    char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static char key[17];

    for (int i = 0; i < keyLength; i++)
    {
        int randomIndex = random(sizeof(charset) - 1);
        key[i] = charset[randomIndex];
    }

    key[keyLength] = '\0';

    return key;
}

// One bounded connection attempt; the main loop services touch between retries.
void xtouch_mqtt_connect()
{
    if (!xtouch_printer_configured(xtouch_active_printer) ||
        (int32_t)(millis() - xtouch_mqtt_nextAttempt) < 0) return;
    XTouchLoopScope timing(XTOUCH_PHASE_CONNECT);
    String clientId = "XTOUCH-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    bool cloud = xtouch_printer_cloud();
    if (cloud && (!xtouch_cloud_ready() || (!xtouch_cloud_simulator() && time(NULL) < 1577836800))) { xtouch_mqtt_nextAttempt=millis()+5000; return; }
    if (cloud && !xtouch_cloud_simulator()) xtouch_wiFiClientSecure.setCACert(xtouch_cloud_trust()); else xtouch_wiFiClientSecure.setInsecure();
    const char *host=cloud?xtouch_cloud_broker():xTouchConfig.xTouchHost;
    uint16_t port=cloud?xtouch_cloud_port():8883;
    xtouch_pubSubClient.setServer(host,port);
    // Keep DFS for idle operation; expensive TLS work uses the configured maximum.
    // This scope stays on the main task, like the existing UI frequency lock.
    struct WorkFrequency { WorkFrequency(){xtouch_power_ui_begin();} ~WorkFrequency(){xtouch_power_ui_end();} };
    xtouch_mqtt_timing.cpuBeforeMHz=getCpuFrequencyMhz();
    WorkFrequency workFrequency;
    xtouch_mqtt_timing.cpuWorkMHz=getCpuFrequencyMhz();
    uint32_t attemptStarted=millis(),stageStarted=attemptStarted;
    ++xtouch_mqtt_timing.attempts;
    bool transportReady = xtouch_wiFiClientSecure.connect(host, port, 2000) == 1;
    xtouch_mqtt_timing.transportMs=millis()-stageStarted;
    xtouch_mqtt_timing.transportReady=transportReady;
    if (transportReady) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        // NetworkClientSecure inherits Stream's millisecond timeout in Arduino 3.
        xtouch_wiFiClientSecure.setTimeout(2000);
#else
        // Arduino 2 WiFiClientSecure overrides this API with seconds.
        xtouch_wiFiClientSecure.setTimeout(2);
#endif
    }
    stageStarted=millis();
    bool connected = transportReady &&
        xtouch_pubSubClient.connect(clientId.c_str(), cloud?xtouch_cloud_user():"bblp", cloud?xtouch_cloud_token():xTouchConfig.xTouchAccessCode);
    xtouch_mqtt_timing.loginMs=millis()-stageStarted;
    xtouch_mqtt_timing.subscribeMs=0;
    if (connected) {
        stageStarted=millis();
        bool subscribed=xtouch_pubSubClient.subscribe(xtouch_mqtt_report_topic.c_str());
        xtouch_mqtt_timing.subscribeMs=millis()-stageStarted;
        if (!subscribed) {
            xtouch_pubSubClient.disconnect();
        } else {
            xtouch_mqtt_retryMs = 5000;
            xtouch_mqtt_lastPushStatus = millis();
            xtouch_device_pushall();
            xtouch_device_get_version();
            ConsoleInfo.printf("[XTouch][PRINTER] slot=%u connected\n", xtouch_active_printer + 1);
        }
    }
    if (!xtouch_pubSubClient.connected()) {
        xtouch_wiFiClientSecure.stop();
        ConsoleError.printf("[XTouch][PRINTER] slot=%u retry state=%d\n",
                            xtouch_active_printer + 1, xtouch_pubSubClient.state());
        if(cloud&&(xtouch_pubSubClient.state()==4||xtouch_pubSubClient.state()==5))xtouch_portal_cloud_rejected();
        xtouch_mqtt_nextAttempt = millis() + ((cloud && (xtouch_pubSubClient.state()==4 || xtouch_pubSubClient.state()==5))?300000:xtouch_mqtt_retryMs);
        xtouch_mqtt_retryMs = min(xtouch_mqtt_retryMs * 2, uint32_t(30000));
    }
    xtouch_mqtt_timing.connected=xtouch_pubSubClient.connected();
    xtouch_mqtt_timing.state=xtouch_pubSubClient.state();
    xtouch_mqtt_timing.totalMs=millis()-attemptStarted;
    ConsoleInfo.printf("[MQTT timing] transport=%u login=%u subscribe=%u total=%u ms cpu=%u->%u state=%d\n",
        unsigned(xtouch_mqtt_timing.transportMs),unsigned(xtouch_mqtt_timing.loginMs),unsigned(xtouch_mqtt_timing.subscribeMs),
        unsigned(xtouch_mqtt_timing.totalMs),xtouch_mqtt_timing.cpuBeforeMHz,xtouch_mqtt_timing.cpuWorkMHz,xtouch_mqtt_timing.state);
    xtouch_mqtt_sendMsg(XTOUCH_ON_PRINT_STATUS);
}

static void xtouch_mqtt_switch_printer()
{
    if (xtouch_pending_printer < 0) return;
    uint8_t slot = xtouch_pending_printer;
    xtouch_pubSubClient.disconnect();
    xtouch_wiFiClientSecure.stop();
    stream.flush();
    xtouch_filament_pending_sequence="";xtouch_device_control_clear();
    xtouch_printer_apply(slot);
    xtouch_pending_printer = -1;
    ui_printer_picker_close();
    memset(&bambuStatus, 0, sizeof(bambuStatus));
    bambuStatus.m_tray_now = 255;
    bambuStatus.m_tray_tar = 255;
    memset(bambuStatus.ams_bound_extruder,255,sizeof(bambuStatus.ams_bound_extruder));
    bambuStatus.nozzle_loaded[0]=bambuStatus.nozzle_loaded[1]=255;
    memset(&controlMode, 0, sizeof(controlMode));
    controlMode.inc = 1;
    lastPrintState = "";
    hms_reset();
    ui_hmsPanel_hide();
    ui_confirmPanel_hide();
    xtouch_mqtt_hasStatus = false;
    xtouch_mqtt_retryMs = 5000;
    // Keep network handshakes from blocking the two-second switch notice.
    xtouch_mqtt_nextAttempt = millis() + 2100;
    xtouch_mqtt_topic_setup();
    xtouch_pubSubClient.setServer(xTouchConfig.xTouchHost, 8883);
    loadScreen(0);
    showPrinterNotice();
    lv_refr_now(NULL);
    ConsoleInfo.printf("[XTouch][PRINTER] selected slot=%u\n", slot + 1);
}

void xtouch_mqtt_setup()
{
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_CONNECTING_PRINTER);
    lv_timer_handler();
    lv_task_handler();
    delay(32);

    xtouch_mqtt_topic_setup();

    xtouch_wiFiClientSecure.flush();
    xtouch_wiFiClientSecure.stop();

    xtouch_wiFiClientSecure.setInsecure();
    xtouch_wiFiClientSecure.setHandshakeTimeout(3);

    xtouch_pubSubClient.setServer(xTouchConfig.xTouchHost, 8883);
    xtouch_pubSubClient.setBufferSize(4096); // 2KB for mqtt message JWT output
    xtouch_pubSubClient.setStream(stream);
    xtouch_pubSubClient.setCallback(xtouch_pubSubClient_streamCallback);
    xtouch_pubSubClient.setKeepAlive(10);
    xtouch_pubSubClient.setSocketTimeout(2);

    /* home */
    lv_msg_subscribe(XTOUCH_COMMAND_LIGHT_TOGGLE, xtouch_message_callback<xtouch_device_onLightToggleCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_STOP, xtouch_message_callback<xtouch_device_onStopCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_PAUSE, xtouch_message_callback<xtouch_device_onPauseCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_RESUME, xtouch_message_callback<xtouch_device_onResumeCommand>, NULL);

    /* control */
    lv_msg_subscribe(XTOUCH_COMMAND_HOME, xtouch_message_callback<xtouch_device_onHomeCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_LEFT, xtouch_message_callback<xtouch_device_onLeftCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_RIGHT, xtouch_message_callback<xtouch_device_onRightCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_UP, xtouch_message_callback<xtouch_device_onUpCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_DOWN, xtouch_message_callback<xtouch_device_onDownCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_BED_TARGET_TEMP, xtouch_message_callback<xtouch_device_onBedTargetTempCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_NOZZLE_TARGET_TEMP, xtouch_message_callback<xtouch_device_onNozzleTargetCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_PART_FAN_SPEED, xtouch_message_callback<xtouch_device_onPartSpeedCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_AUX_FAN_SPEED, xtouch_message_callback<xtouch_device_onAuxSpeedCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_CHAMBER_FAN_SPEED, xtouch_message_callback<xtouch_device_onChamberSpeedCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_PRINT_SPEED, xtouch_message_callback<xtouch_device_onPrintSpeedCommand>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_NOZZLE_INDEXED,xtouch_device_onIndexedNozzle,NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_AMS_DRY,xtouch_device_onAmsDry,NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_FILAMENT_ACTION, xtouch_device_onFilamentAction, NULL);

    lv_msg_subscribe(XTOUCH_COMMAND_AMS_CONTROL, xtouch_device_command_ams_control, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_CLEAN_PRINT_ERROR, xtouch_device_command_clean_print_error, NULL);

    /* filament */
    lv_msg_subscribe(XTOUCH_COMMAND_EXTRUDE_UP, xtouch_message_callback<xtouch_device_onNozzleUp>, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_EXTRUDE_DOWN, xtouch_message_callback<xtouch_device_onNozzleDown>, NULL);

    loadScreen(0);
    xtouch_screen_startScreenTimer();
    xtouch_mqtt_nextAttempt = millis() + 150;
}

void xtouch_mqtt_loop()
{
    xtouch_mqtt_switch_printer();
    if (xtouch_pubSubClient.connected()) {
        {XTouchLoopScope timing(XTOUCH_PHASE_RECEIVE);xtouch_pubSubClient.loop();}
        if ((uint32_t)(millis() - xtouch_mqtt_lastPushStatus) >
            (xtouch_printer_cloud()?180U:XTOUCH_MQTT_SERVER_PUSH_STATUS_TIMEOUT) * 1000U) {
            xtouch_pubSubClient.disconnect();
            xtouch_mqtt_hasStatus = false;
            xtouch_mqtt_nextAttempt = millis() + 5000;
            xtouch_mqtt_sendMsg(XTOUCH_ON_PRINT_STATUS);
        }
    } else if (lv_disp_get_inactive_time(NULL) >= (xTouchConfig.currentScreenIndex==4?3000u:500u) &&
               !ui_scroll_busy(3000)) {
        // A short pause between settings-page swipes is still interaction,
        // not an opportunity to start a blocking TLS handshake.
        xtouch_mqtt_connect();
    }
    delay(1);
}

#endif
