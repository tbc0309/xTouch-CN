#ifndef _XLCD_TYPES
#define _XLCD_TYPES
#include <stdbool.h>
#include <stdint.h>

#define XTOUCH_LCD_MIN_SLEEP_TIME 5
/* Four four-slot units plus eight one-slot HT units; protocol IDs stay separate. */
#define XTOUCH_STANDARD_AMS_UNITS 4
#define XTOUCH_HT_AMS_UNITS 8
#define XTOUCH_STANDARD_AMS_SLOTS (XTOUCH_STANDARD_AMS_UNITS * 4)
#define XTOUCH_MIXED_AMS_UNIT (XTOUCH_STANDARD_AMS_UNITS + XTOUCH_HT_AMS_UNITS)
#define XTOUCH_MAX_AMS_UNITS (XTOUCH_MIXED_AMS_UNIT + 1)
#define XTOUCH_AMS_SLOTS_PER_UNIT 4
#define XTOUCH_MAX_AMS_SLOTS (XTOUCH_MAX_AMS_UNITS * XTOUCH_AMS_SLOTS_PER_UNIT)

#ifdef __cplusplus
extern "C"
{
#endif

    enum XTouchPrinterSeries {
        SERIES_X1 = 0,
        SERIES_P1P,
        SERIES_UNKNOWN,
    };

    struct XTouchPanelConfig
    {
        float xCalM;
        float yCalM;
        float xCalC;
        float yCalC;
    };

    enum XTouchPrintStatus
    {
        XTOUCH_PRINT_STATUS_IDLE,
        XTOUCH_PRINT_STATUS_RUNNING,
        XTOUCH_PRINT_STATUS_PAUSED,
        XTOUCH_PRINT_STATUS_FINISHED,
        XTOUCH_PRINT_STATUS_PREPARE,
        XTOUCH_PRINT_STATUS_FAILED,
    };

    enum XTouchPrintingSpeedLevel
    {
        XTOUCH_SPEED_LEVEL_INVALID = 0,
        XTOUCH_SPEED_LEVEL_SILENCE = 1,
        XTOUCH_SPEED_LEVEL_NORMAL = 2,
        XTOUCH_SPEED_LEVEL_RAPID = 3,
        XTOUCH_SPEED_LEVEL_RAMPAGE = 4,
        XTOUCH_SPEED_LEVEL_COUNT
    };

    enum XTouchAmsStatusMain
    {
        AMS_STATUS_MAIN_IDLE = 0x00,
        AMS_STATUS_MAIN_FILAMENT_CHANGE = 0x01,
        AMS_STATUS_MAIN_RFID_IDENTIFYING = 0x02,
        AMS_STATUS_MAIN_ASSIST = 0x03,
        AMS_STATUS_MAIN_CALIBRATION = 0x04,
        AMS_STATUS_MAIN_SELF_CHECK = 0x10,
        AMS_STATUS_MAIN_DEBUG = 0x20,
        AMS_STATUS_MAIN_UNKNOWN = 0xFF,
    };

    typedef struct BambuMQTTPayload
    {
        char print_type[32];
        int home_flag;
        int hw_switch_state;
        int mc_left_time;
        int mc_print_percent;
        int mc_print_sub_stage;
        int mc_print_stage;
        int mc_print_error_code;
        int mc_print_line_number;
        int print_error;
        char printer_type[32];
        char subtask_name[32];
        int current_layer;
        bool current_layer_valid;
        int total_layers;
        int print_status;
        int queue_number;
        int gcode_file_prepare_percent;
        char obj_subtask_id[32];
        char project_id_[32];
        char profile_id_[32];
        char subtask_id_[32];
        char gcode_file[128];
        int plate_index;
        bool plate_index_valid;
        char task_id[32];
        unsigned char temperature_valid,fan_valid,fan_support_known,fan_supported;
        bool bed_mqtt_control;
        uint8_t fan_protocol_ids[3],fan_protocol_mask;
        bool chamber_support_known,chamber_supported,chamber_edit_known,chamber_edit_supported,fan_new_protocol;
        unsigned char chamber_edit_min,chamber_edit_max;
        double bed_temper;
        double bed_target_temper;
        double frame_temp;
        double nozzle_temper;
        double nozzle_target_temper;
        double chamber_temper;
        double chamber_target_temper;
        bool chamber_valid, chamber_target_valid;
        int wifi_signal;
        bool wifi_signal_valid;
        int cooling_fan_speed;
        int big_fan1_speed;
        int big_fan2_speed;
        int printing_speed_lvl;
        int printing_speed_mag;
        bool chamberLed;
        float nozzle_diameter;
        bool camera_recording_when_printing; // Automatic-recording setting, not activity.
        bool camera_recording;
        bool camera_recording_valid;
        bool camera_status_packed, camera_settings_packed;
        bool camera_timelapse;
        bool has_ipcam;
        long int ams_exist_bits;
        bool ams;
        // Fixed-size AMS snapshot. Missing delta fields keep their previous value.
        // Optional telemetry uses validity masks: missing/unsupported is never 0%.
        uint64_t ams_remain_valid;
        uint8_t ams_remain_percent[XTOUCH_MAX_AMS_SLOTS];
        uint16_t ams_humidity_valid;
        uint8_t ams_humidity_percent[XTOUCH_MAX_AMS_UNITS];
        uint16_t ams_temperature_valid;
        int16_t ams_temperature_deci[XTOUCH_MAX_AMS_UNITS];
        uint16_t ams_nozzle_min[XTOUCH_MAX_AMS_SLOTS];
        uint16_t ams_nozzle_max[XTOUCH_MAX_AMS_SLOTS];
        uint32_t external_color;
        bool external_present, external_multi_protocol;
        uint8_t external_routes, external_materials;
        uint32_t external_color2;
        uint16_t external_nozzle_min2, external_nozzle_max2;
        uint16_t external_nozzle_min;
        uint16_t external_nozzle_max;
        uint32_t ams_tray_color[XTOUCH_MAX_AMS_SLOTS];
        bool ams_tray_color_valid[XTOUCH_MAX_AMS_SLOTS];
        char ams_tray_material[XTOUCH_MAX_AMS_SLOTS][12];
        uint8_t ams_kind[XTOUCH_MAX_AMS_UNITS]; // 0 unknown, 1 AMS, 2 Lite, 3 Pro, 4 HT
        uint8_t ams_module_kind[XTOUCH_MAX_AMS_UNITS]; // get_version may precede presence reports
        uint8_t ams_dry_status[XTOUCH_MAX_AMS_UNITS];
        uint8_t ams_bound_extruder[XTOUCH_MAX_AMS_UNITS];
        uint16_t ams_dry_valid;
        uint16_t ams_dry_minutes[XTOUCH_MAX_AMS_UNITS];
        uint8_t ams_dry_target[XTOUCH_MAX_AMS_UNITS];
        uint8_t ams_dry_hours[XTOUCH_MAX_AMS_UNITS];
        uint8_t nozzle_valid, nozzle_count, active_nozzle, nozzle_loaded_valid;
        bool extruder_busy, rack_present, non_fdm_mode;
        uint8_t rack_status, rack_position, rack_reading, mixed_ams_protocol_id;
        uint16_t rack_mask;
        uint8_t rack_diameter[16];
        char rack_type[16][16];
        uint16_t nozzle_actual[2], nozzle_target[2];
        int16_t nozzle_loaded[2];
        int ams_status_sub;
        int ams_status_main;
        long int ams_version;
        bool ams_support_use_ams;
        int ams_rfid_status;
        int ams_humidity;
        int ams_user_setting_hold_count;
        bool ams_insert_flag;
        bool ams_power_on_flag;
        bool ams_calibrate_remain_flag;
        bool ams_support_virtual_tray;
        bool is_ams_need_update;
        uint64_t tray_exist_bits;
        uint64_t tray_is_bbl_bits;
        uint64_t tray_read_done_bits;
        uint64_t tray_reading_bits;
        int m_ams_id;   // Dense UI unit index; translate before sending commands.
        int m_tray_id;  // local tray id : "0" ~ "3"
        int m_tray_now; // Dense UI slot, external 254, empty 255, unknown -1.
        int m_tray_tar; // Same normalized slot representation.

    } XTouchBambuStatus;

    extern XTouchBambuStatus bambuStatus;

    enum XTouchControlAxis
    {
        ControlAxisXY,
        ControlAxisZ,
    };

    typedef struct XTouchControlModeStruct
    {
        int inc;
        int axis;
        int target_bed_temper;
        int target_nozzle_temper;
    } XtouchControlMode;

    extern XtouchControlMode controlMode;

    typedef struct XTouchTouchConfigStruct
    {
        bool xTouchAuxFanEnabled;
        bool xTouchChamberFanEnabled;
        bool xTouchChamberSensorEnabled;
        int8_t xTouchChamberSensorReadingDiff;
        bool xTouchOTAEnabled;
        bool xTouchTFTFlip;
        bool xTouchTFTInvert;
        bool xTouchPrinterPickerList;
        char xTouchHost[64];
        char xTouchAccessCode[9];
        char xTouchSerialNumber[32];
        char xTouchPrinterModel[32];
        char xTouchPrinterName[64];
        int xTouchBacklightLevel;
        int xTouchTFTOFFValue;
        bool xTouchWakeOnPrint;
        bool xTouchTouchSound,xTouchWarningSound,xTouchStatusLed;
        uint8_t xTouchSoundVolume;
        bool xTouchBrowserTimezoneSync;
        char xTouchTimezone[96],xTouchTimezoneName[64],xTouchNtpServer[128];
        char xTouchPortalLanguage[8];
        int xTouchDeepSleepAfter;
        int currentScreenIndex;
    } XTouchConfig;

    extern XTouchConfig xTouchConfig;

    typedef struct
    {
        char subtask_id[32];
        int print_error;
    } ClearErrorMessage;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
