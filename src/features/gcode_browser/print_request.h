#pragma once
#include "policy.h"
#include <ArduinoJson.h>
namespace xtouch_files {
inline bool printRequest(JsonDocument &doc, const Entry &entry, unsigned plate, const char *model,
                         const char *sequence) {
    if (!pathValid(entry.path) || !printable(entry.path) || plate < 1 || plate > 999 || !model ||
        !*model)
        return false;
    auto print = doc.createNestedObject("print");
    print["sequence_id"] = sequence;
    if (!archive(entry.path)) {
        print["command"] = "gcode_file";
        print["param"] = entry.path;
        return !doc.overflowed();
    }
    print["command"] = "project_file";
    print["file"] = entry.path.substr(entry.path.find_last_of('/') + 1);
    print["md5"] = "from_sd_card";
    print["param"] = std::string("Metadata/plate_") + std::to_string(plate) + ".gcode";
    std::string url =
        (!strncmp(model, "H2", 2) || !strcmp(model, "P2S")) ? "ftp://" : "file:///sdcard";
    for (unsigned char c : entry.path) {
        if (c == '/' || isalnum(c) || c == '-' || c == '_' || c == '.')
            url += char(c);
        else {
            char encoded[4];
            snprintf(encoded, sizeof(encoded), "%%%02X", c);
            url += encoded;
        }
    }
    print["url"] = url;
    print["bed_type"] = "auto";
    print["bed_leveling"] = true;
    print["use_ams"] = false;
    print.createNestedArray("ams_mapping");
    print.createNestedArray("ams_mapping2");
    print["timelapse"] = false;
    print["flow_cali"] = false;
    print["vibration_cali"] = false;
    print["layer_inspect"] = false;
    print["subtask_name"] = entry.path.substr(entry.path.find_last_of('/') + 1);
    for (const char *id : {"profile_id", "project_id", "subtask_id", "task_id"})
        print[id] = "0";
    return !doc.overflowed();
}
} // namespace xtouch_files
