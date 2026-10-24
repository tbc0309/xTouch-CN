#pragma once
#include "ams_units.h"
#include <limits.h>
// AMS unit IDs and per-slot masks use separate, bounded mappings.
static void xtouch_mqtt_parse_ams(JsonDocument &incomingJson) {
    // AMS reports are deltas under print.ams, not at the document root.
    JsonObjectConst ams = incomingJson["print"]["ams"].as<JsonObjectConst>();
    if (!ams.isNull()) {
        auto integer = [](JsonVariantConst v) -> long {
            if (v.is<const char *>()) {
                char *end;
                const char *s = v.as<const char *>();
                long n = strtol(s, &end, 10);
                return *s && !*end ? n : LONG_MIN;
            }
            return v.is<long>() ? v.as<long>() : LONG_MIN;
        };
        auto number = [](JsonVariantConst v, double &value) -> bool {
            if (v.is<const char *>()) {
                const char *text = v.as<const char *>();
                char *end;
                if (!text || !*text)
                    return false;
                value = strtod(text, &end);
                return *end == 0 && isfinite(value);
            }
            if (!v.is<double>() && !v.is<long>())
                return false;
            value = v.as<double>();
            return isfinite(value);
        };
        // Presence masks are hexadecimal strings; tray numbers are decimal.
        auto bits = [](JsonVariantConst v) -> uint32_t {
            if (v.is<const char *>()) {
                const char *s = v.as<const char *>();
                size_t n = strlen(s);
                if (!n || n > 8)
                    return 0;
                for (size_t i = 0; i < n; i++)
                    if (!isxdigit((unsigned char)s[i]))
                        return 0;
                return strtoul(s, nullptr, 16);
            }
            return v.is<uint32_t>() ? v.as<uint32_t>() : 0;
        };
        if (ams.containsKey("ams_exist_bits"))
            bambuStatus.ams_exist_bits =
                bits(ams["ams_exist_bits"]) & ((1U << XTOUCH_MAX_AMS_UNITS) - 1);
        if (ams.containsKey("tray_exist_bits")) {
            bambuStatus.tray_exist_bits = xtouch_ams_slot_mask(bits(ams["tray_exist_bits"]));
            for (unsigned i = 0; i < XTOUCH_MAX_AMS_SLOTS; ++i)
                if (!(bambuStatus.tray_exist_bits & (1ULL << i))) {
                    bambuStatus.ams_tray_color_valid[i] = false;
                    bambuStatus.ams_tray_material[i][0] = 0;
                    bambuStatus.ams_nozzle_min[i] = bambuStatus.ams_nozzle_max[i] = 0;
                    bambuStatus.ams_remain_valid &= ~(1ULL << i);
                }
        }
        if (ams.containsKey("tray_now")) {
            bambuStatus.m_tray_now = xtouch_ams_tray_index(integer(ams["tray_now"]));
            char value[12];
            snprintf(value, sizeof(value), "%d", bambuStatus.m_tray_now);
            xtouch_ams_parse_tray_now(value);
        }
        if (ams.containsKey("tray_tar"))
            bambuStatus.m_tray_tar = xtouch_ams_tray_index(integer(ams["tray_tar"]));
        if (ams.containsKey("version"))
            bambuStatus.ams_version = integer(ams["version"]);
        if (ams.containsKey("tray_is_bbl_bits"))
            bambuStatus.tray_is_bbl_bits = xtouch_ams_slot_mask(bits(ams["tray_is_bbl_bits"]));
        if (ams.containsKey("tray_read_done_bits"))
            bambuStatus.tray_read_done_bits =
                xtouch_ams_slot_mask(bits(ams["tray_read_done_bits"]));
        if (ams.containsKey("tray_reading_bits"))
            bambuStatus.tray_reading_bits = xtouch_ams_slot_mask(bits(ams["tray_reading_bits"]));
        JsonArrayConst units = ams["ams"].as<JsonArrayConst>();
        int mixedSeen = -1;
        bool mixedConflict = false;
        for (JsonObjectConst unit : units) {
            if (!unit.containsKey("id"))
                continue;
            long rawId = integer(unit["id"]);
            int id = xtouch_ams_unit_index(rawId);
            if (id < 0 || id >= XTOUCH_MAX_AMS_UNITS)
                continue;
            if (id == XTOUCH_MIXED_AMS_UNIT) {
                if (mixedSeen >= 0 && mixedSeen != rawId)
                    mixedConflict = true;
                mixedSeen = rawId;
                bambuStatus.mixed_ams_protocol_id = rawId;
            }
            if (!ams.containsKey("ams_exist_bits"))
                bambuStatus.ams_exist_bits |= 1ULL << id;
            if(!bambuStatus.ams_kind[id]&&bambuStatus.ams_module_kind[id])
                bambuStatus.ams_kind[id]=bambuStatus.ams_module_kind[id];
            if (id >= 4)
                bambuStatus.ams_kind[id] = id == XTOUCH_MIXED_AMS_UNIT ? 5 : 4;
            if (unit.containsKey("info")) {
                uint32_t flags = bits(unit["info"]);
                unsigned kind = flags & 15;
                bambuStatus.ams_kind[id] = (kind >= 1 && kind <= 5 &&
                                            ((id < 4 && kind <= 3) ||
                                             (id >= 4 && id < XTOUCH_MIXED_AMS_UNIT && kind == 4) ||
                                             (id == XTOUCH_MIXED_AMS_UNIT && kind == 5)))
                                               ? kind
                                               : 255;
                bambuStatus.ams_dry_status[id] = (flags >> 4) & 15;
                bambuStatus.ams_bound_extruder[id] = (flags >> 8) & 15;
            }
            if (id == XTOUCH_MIXED_AMS_UNIT && mixedConflict)
                bambuStatus.ams_kind[id] = 255;
            if (unit.containsKey("dry_time")) {
                long minutes = integer(unit["dry_time"]);
                if (minutes >= 0 && minutes <= 2880) {
                    bambuStatus.ams_dry_minutes[id] = minutes;
                    bambuStatus.ams_dry_valid |= 1U << id;
                } else
                    bambuStatus.ams_dry_valid &= ~(1U << id);
            }
            JsonObjectConst dry = unit["dry_setting"].as<JsonObjectConst>();
            if (dry.containsKey("dry_temperature")) {
                long n = integer(dry["dry_temperature"]);
                bambuStatus.ams_dry_target[id] = n >= 0 && n <= 85 ? n : 0;
            }
            if (dry.containsKey("dry_duration")) {
                long n = integer(dry["dry_duration"]);
                bambuStatus.ams_dry_hours[id] = n >= 0 && n <= 24 ? n : 0;
            }
            if (unit.containsKey("humidity"))
                bambuStatus.ams_humidity = integer(unit["humidity"]);
            double value;
            if (unit.containsKey("humidity_raw")) {
                if (number(unit["humidity_raw"], value) && value >= 0 && value <= 100) {
                    bambuStatus.ams_humidity_percent[id] = value;
                    bambuStatus.ams_humidity_valid |= 1ULL << id;
                } else
                    bambuStatus.ams_humidity_valid &= ~(1ULL << id);
            }
            if (unit.containsKey("temp")) {
                if (number(unit["temp"], value) && value >= -20 && value <= 120) {
                    bambuStatus.ams_temperature_deci[id] = lround(value * 10);
                    bambuStatus.ams_temperature_valid |= 1ULL << id;
                } else
                    bambuStatus.ams_temperature_valid &= ~(1ULL << id);
            }
            for (JsonObjectConst tray : unit["tray"].as<JsonArrayConst>()) {
                if (!tray.containsKey("id"))
                    continue;
                int slot = integer(tray["id"]);
                if (slot < 0 || (unsigned)slot >= xtouch_ams_slot_count(id))
                    continue;
                unsigned index = id * 4 + slot;
                if (tray.containsKey("remain")) {
                    if (number(tray["remain"], value) && value >= 0 && value <= 100) {
                        bambuStatus.ams_remain_percent[index] = value;
                        bambuStatus.ams_remain_valid |= 1ULL << index;
                    } else
                        bambuStatus.ams_remain_valid &= ~(1ULL << index);
                }
                if (tray.containsKey("nozzle_temp_min"))
                    bambuStatus.ams_nozzle_min[index] =
                        number(tray["nozzle_temp_min"], value) && value >= 150 && value <= 350
                            ? value
                            : 0;
                if (tray.containsKey("nozzle_temp_max"))
                    bambuStatus.ams_nozzle_max[index] =
                        number(tray["nozzle_temp_max"], value) && value >= 150 && value <= 350
                            ? value
                            : 0;
                if (tray.containsKey("tray_color")) {
                    const char *hex = tray["tray_color"] | "";
                    bool valid = strlen(hex) == 8;
                    for (unsigned c = 0; valid && c < 8; ++c)
                        valid = isxdigit((unsigned char)hex[c]);
                    bambuStatus.ams_tray_color_valid[index] = valid;
                    if (valid)
                        bambuStatus.ams_tray_color[index] = strtoul(hex, nullptr, 16) >> 8;
                }
                if (tray.containsKey("tray_type"))
                    strlcpy(bambuStatus.ams_tray_material[index], tray["tray_type"] | "",
                            sizeof(bambuStatus.ams_tray_material[index]));
            }
        }
        if (ams.containsKey("ams") && units.size() == 0 && !ams.containsKey("ams_exist_bits"))
            bambuStatus.ams_exist_bits = 0;
        if (ams.containsKey("ams_exist_bits") || ams.containsKey("ams"))
            bambuStatus.ams = bambuStatus.ams_exist_bits != 0;
        if (!bambuStatus.ams) {
            memset(bambuStatus.ams_tray_color_valid, 0, sizeof(bambuStatus.ams_tray_color_valid));
            memset(bambuStatus.ams_tray_material, 0, sizeof(bambuStatus.ams_tray_material));
        }
        // Removing a unit invalidates its optional readings and all four slots.
        bambuStatus.ams_humidity_valid &= bambuStatus.ams_exist_bits;
        bambuStatus.ams_temperature_valid &= bambuStatus.ams_exist_bits;
        bambuStatus.ams_dry_valid &= bambuStatus.ams_exist_bits;
        for (unsigned id = 0; id < XTOUCH_MAX_AMS_UNITS; ++id) {
            if (bambuStatus.ams_exist_bits & (1U << id))
                continue;
            bambuStatus.ams_bound_extruder[id] = 255;
            bambuStatus.ams_kind[id] = 0;
            bambuStatus.ams_dry_minutes[id] = 0;
            bambuStatus.ams_dry_target[id] = 0;
            bambuStatus.ams_dry_status[id] = 0;
            bambuStatus.ams_remain_valid &= ~(0xFULL << (id * 4));
            bambuStatus.tray_exist_bits &= ~(0xFULL << (id * 4));
            for (unsigned slot = id * 4; slot < id * 4 + 4; ++slot) {
                bambuStatus.ams_nozzle_min[slot] = bambuStatus.ams_nozzle_max[slot] = 0;
                bambuStatus.ams_tray_color_valid[slot] = false;
                bambuStatus.ams_tray_material[slot][0] = 0;
            }
        }
        bambuStatus.is_ams_need_update = true;
        xtouch_mqtt_sendMsg(XTOUCH_ON_AMS, bambuStatus.ams);
        xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS, 0);
    }
}
