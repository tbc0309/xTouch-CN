#pragma once
namespace pixel_screen {
// Warm-up demand/stage, not an undocumented CTC heater relay bit.
template<class Status> inline bool chamberWarming(const Status &s){
    if(s.chamber_target_valid&&s.chamber_target_temper<=0)return false;
    if(s.chamber_valid&&s.chamber_target_valid){
        if(s.chamber_temper>=s.chamber_target_temper-1)return false;
        return s.print_stage_current==49||(s.chamber_edit_known&&s.chamber_edit_supported);
    }
    return s.print_stage_current==49;
}
}
