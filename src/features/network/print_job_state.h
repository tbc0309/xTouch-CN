#pragma once
#include <cstring>

namespace XTouchPrintJob {
inline bool active(const char *state) {
    return state && (!std::strcmp(state, "PREPARE") || !std::strcmp(state, "RUNNING") ||
                     !std::strcmp(state, "PAUSE"));
}
inline bool terminal(const char *state) {
    return state && (!std::strcmp(state, "IDLE") || !std::strcmp(state, "FINISH") ||
                     !std::strcmp(state, "FAILED"));
}
inline bool known(const char *value) {
    return value && *value && std::strcmp(value, "0");
}
inline bool same(const char *before, const char *after) {
    return known(before) && known(after) && !std::strcmp(before, after);
}
inline bool changed(const char *before, const char *after) {
    return known(before) && known(after) && std::strcmp(before, after);
}
inline bool starts(const char *before, const char *after, bool identityChanged) {
    if (!active(after))
        return false;
    if (!active(before))
        return true;
    // PREPARE reports progressively add the new job's identity and metadata.
    // Neither enrichment nor the transition to RUNNING starts another job.
    if (!std::strcmp(after, "PREPARE"))
        return std::strcmp(before, "PREPARE") && std::strcmp(before, "PAUSE");
    if (!std::strcmp(before, "PREPARE"))
        return false;
    return identityChanged;
}
template <class Status> inline void clear(Status &s) {
    s.mc_left_time = -1;
    s.mc_print_percent = 0;
    s.current_layer = 0;
    s.current_layer_valid = false;
    s.total_layers = 0;
    s.mc_print_stage = 0;
    s.mc_print_sub_stage = 0;
    s.mc_print_line_number = 0;
    s.gcode_file_prepare_percent = 0;
}
template <class Status>
inline bool beginReport(Status &s, const char *before, const char *after, bool identityChanged) {
    bool started = starts(before, after, identityChanged);
    if (started) {
        clear(s);
        s.task_id[0] = s.obj_subtask_id[0] = s.gcode_file[0] = s.subtask_name[0] = 0;
        s.project_id_[0] = s.profile_id_[0] = s.subtask_id_[0] = 0;
        s.plate_index = 1;
        s.plate_index_valid = false;
    }
    return started;
}
template <class Status> inline void finishReport(Status &s, const char *state) {
    if (terminal(state)) {
        clear(s);
    } else if (state && !std::strcmp(state, "PREPARE")) {
        s.mc_print_percent = 0;
        if (s.current_layer > 0) {
            s.current_layer = 0;
            s.current_layer_valid = false;
        }
    }
}
} // namespace XTouchPrintJob
