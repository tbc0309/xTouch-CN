#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
struct ui_dry_request {
    unsigned printer, unit, temp, hours;
    bool stop;
};
void ui_ams_details_show(unsigned unit);
bool ui_ams_dry_allowed(const struct ui_dry_request *request);
#ifdef __cplusplus
}
#endif
